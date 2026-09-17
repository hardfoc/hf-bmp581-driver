/**
 * @file bmp581_driver.hpp
 * @brief Bosch BMP581 barometric pressure sensor client (I2C or SPI via CRTP bus).
 *
 * @details Ported from the register sequences of the Bosch BMP5 Sensor API
 *          (v1.1.1, BSD-3) into a header-only C++17 driver: chip-id / NVM-ready
 *          init, soft reset with POR check, power modes with the mandatory
 *          standby hop, OSR/ODR, IIR (standby-only write, mode restored),
 *          interrupt pin / source configuration, data-ready polling, 24-bit
 *          pressure (1/64 Pa) and temperature (1/65536 °C) read, and NVM
 *          user-word read. FIFO is intentionally out of scope.
 *
 *          Typical session:
 *          @code
 *          bmp581::Driver<MyBus> dev(bus);
 *          dev.Init();                                  // chip id + NVM ready
 *          dev.SoftReset();
 *          dev.Configure(bmp581::BarometricReferencePreset());
 *          if (dev.DataReady().value) { auto s = dev.ReadSample(); }
 *          @endcode
 *
 * @copyright Copyright (c) 2026 HardFOC. All rights reserved.
 */
#pragma once

#include "bmp581_bus_interface.hpp"
#include "bmp581_registers.hpp"
#include "bmp581_types.hpp"

#include <cstddef>
#include <cstdint>

namespace bmp581 {

/**
 * @brief BMP581 client.
 * @tparam BusT Concrete type inheriting `BusInterface<BusT>`.
 */
template <typename BusT>
class Driver {
public:
    explicit Driver(BusT& bus) noexcept : bus_(bus) {}

    /// Chip id read by the last successful `Init()` (0 before).
    uint8_t ChipId() const noexcept { return chip_id_; }
    bool Initialized() const noexcept { return chip_id_ != 0; }

    // ------------------------------------------------------------------ lifecycle

    /**
     * @brief Verify chip id (0x50 / 0x51) and NVM readiness.
     * @note For SPI adapters the first read after power-up may return junk;
     *       the adapter should perform one dummy read, or call `Init()` twice.
     */
    DriverResult<void> Init() noexcept {
        chip_id_ = 0;
        if (!bus_.EnsureInitialized()) {
            return DriverResult<void>::failure(DriverError::NotInitialized);
        }
        uint8_t id = 0;
        if (!bus_.ReadRegs(reg::kChipId, &id, 1)) {
            return DriverResult<void>::failure(DriverError::Bus);
        }
        if (id == 0) {
            return DriverResult<void>::failure(DriverError::InvalidChipId);
        }
        const DriverError nvm = NvmReadyCheck();
        if (nvm != DriverError::None) {
            return DriverResult<void>::failure(nvm);
        }
        if (id != chip_id::kPrimary && id != chip_id::kSecondary) {
            return DriverResult<void>::failure(DriverError::InvalidChipId);
        }
        chip_id_ = id;
        return DriverResult<void>::success();
    }

    /// CMD = 0xB6, wait 2 ms, confirm INT_STATUS.por_softreset_complete.
    DriverResult<void> SoftReset() noexcept {
        if (!WriteReg(reg::kCmd, cmd::kSoftReset)) {
            return DriverResult<void>::failure(DriverError::Bus);
        }
        bus_.DelayUs(timing::kSoftResetUs);
        uint8_t st = 0;
        if (!bus_.ReadRegs(reg::kIntStatus, &st, 1)) {
            return DriverResult<void>::failure(DriverError::Bus);
        }
        if ((st & int_status::kPorSoftResetComplete) == 0U) {
            return DriverResult<void>::failure(DriverError::PorSoftReset);
        }
        return DriverResult<void>::success();
    }

    /// Soft reset → OSR/ODR → IIR → power mode, in the order the device requires.
    DriverResult<void> Configure(const Config& cfg) noexcept {
        auto r = SetPowerMode(PowerMode::Standby);
        if (!r.ok()) return r;
        r = SetOsrOdr(cfg.osr_odr);
        if (!r.ok()) return r;
        r = SetIir(cfg.iir);
        if (!r.ok()) return r;
        return SetPowerMode(cfg.power_mode);
    }

    // ---------------------------------------------------------------- power mode

    DriverResult<PowerMode> GetPowerMode() noexcept {
        uint8_t odr = 0;
        if (!bus_.ReadRegs(reg::kOdrConfig, &odr, 1)) {
            return DriverResult<PowerMode>::failure(DriverError::Bus);
        }
        const uint8_t mode = odr & odr_config::kPowerModeMask;
        if (mode == static_cast<uint8_t>(PowerMode::Standby)) {
            const bool deep_disabled = (odr & odr_config::kDeepDisableMask) != 0U;
            return DriverResult<PowerMode>::success(deep_disabled ? PowerMode::Standby
                                                                  : DeepStandbyOrStandby());
        }
        return DriverResult<PowerMode>::success(static_cast<PowerMode>(mode));
    }

    /**
     * @brief Set power mode. Always hops through standby (t_standby 2.5 ms) as
     *        the data sheet requires before entering Normal/Forced/Continuous.
     */
    DriverResult<void> SetPowerMode(PowerMode mode) noexcept {
        const auto cur = GetPowerMode();
        if (!cur.ok()) {
            return DriverResult<void>::failure(cur.error);
        }
        if (cur.value != PowerMode::Standby) {
            if (!WritePowerModeBits(PowerMode::Standby, /*deep_disable=*/true)) {
                return DriverResult<void>::failure(DriverError::Bus);
            }
            bus_.DelayUs(timing::kStandbyUs);
        }
        switch (mode) {
            case PowerMode::Standby:
                return DriverResult<void>::success();
            case PowerMode::DeepStandby:
                return WritePowerModeBits(PowerMode::Standby, /*deep_disable=*/false)
                           ? DriverResult<void>::success()
                           : DriverResult<void>::failure(DriverError::Bus);
            case PowerMode::Normal:
            case PowerMode::Forced:
            case PowerMode::Continuous:
                return WritePowerModeBits(mode, /*deep_disable=*/true)
                           ? DriverResult<void>::success()
                           : DriverResult<void>::failure(DriverError::Bus);
        }
        return DriverResult<void>::failure(DriverError::InvalidPowerMode);
    }

    // ------------------------------------------------------------- OSR / ODR / IIR

    DriverResult<OsrOdrConfig> GetOsrOdr() noexcept {
        uint8_t r[2]{};
        if (!bus_.ReadRegs(reg::kOsrConfig, r, 2)) {
            return DriverResult<OsrOdrConfig>::failure(DriverError::Bus);
        }
        OsrOdrConfig c{};
        c.osr_temperature = static_cast<Oversampling>(r[0] & osr_config::kTempOsMask);
        c.osr_pressure = static_cast<Oversampling>((r[0] & osr_config::kPressOsMask) >>
                                                   osr_config::kPressOsShift);
        c.pressure_enable = (r[0] & osr_config::kPressEnMask) != 0U;
        c.odr = static_cast<Odr>((r[1] & odr_config::kOdrMask) >> odr_config::kOdrShift);
        return DriverResult<OsrOdrConfig>::success(c);
    }

    /**
     * @brief Write OSR_CONFIG / ODR_CONFIG preserving power-mode bits.
     * ODR faster than 5 Hz is only valid with deep standby disabled; the driver
     * forces the device to plain standby first in that case (Bosch API behaviour).
     */
    DriverResult<void> SetOsrOdr(const OsrOdrConfig& c) noexcept {
        if (static_cast<uint8_t>(c.odr) < static_cast<uint8_t>(Odr::Hz5)) {
            const auto cur = GetPowerMode();
            if (!cur.ok()) {
                return DriverResult<void>::failure(cur.error);
            }
            if (cur.value == PowerMode::DeepStandby) {
                if (!WritePowerModeBits(PowerMode::Standby, true)) {
                    return DriverResult<void>::failure(DriverError::Bus);
                }
            }
        }
        uint8_t r[2]{};
        if (!bus_.ReadRegs(reg::kOsrConfig, r, 2)) {
            return DriverResult<void>::failure(DriverError::Bus);
        }
        r[0] = static_cast<uint8_t>((r[0] & ~(osr_config::kTempOsMask | osr_config::kPressOsMask |
                                              osr_config::kPressEnMask)) |
                                    (static_cast<uint8_t>(c.osr_temperature) & osr_config::kTempOsMask) |
                                    ((static_cast<uint8_t>(c.osr_pressure) << osr_config::kPressOsShift) &
                                     osr_config::kPressOsMask) |
                                    (c.pressure_enable ? osr_config::kPressEnMask : 0U));
        r[1] = static_cast<uint8_t>((r[1] & ~odr_config::kOdrMask) |
                                    ((static_cast<uint8_t>(c.odr) << odr_config::kOdrShift) &
                                     odr_config::kOdrMask));
        return bus_.WriteRegs(reg::kOsrConfig, r, 2) ? DriverResult<void>::success()
                                                     : DriverResult<void>::failure(DriverError::Bus);
    }

    DriverResult<EffectiveOsr> GetEffectiveOsr() noexcept {
        uint8_t v = 0;
        if (!bus_.ReadRegs(reg::kOsrEff, &v, 1)) {
            return DriverResult<EffectiveOsr>::failure(DriverError::Bus);
        }
        EffectiveOsr e{};
        e.osr_temperature = static_cast<Oversampling>(v & osr_eff::kTempOsMask);
        e.osr_pressure = static_cast<Oversampling>((v & osr_eff::kPressOsMask) >> osr_eff::kPressOsShift);
        e.odr_valid = (v & osr_eff::kOdrValidMask) != 0U;
        return DriverResult<EffectiveOsr>::success(e);
    }

    DriverResult<IirConfig> GetIir() noexcept {
        uint8_t r[2]{};
        if (!bus_.ReadRegs(reg::kDspConfig, r, 2)) {
            return DriverResult<IirConfig>::failure(DriverError::Bus);
        }
        IirConfig c{};
        c.shadow_iir_temperature = (r[0] & dsp_config::kShadowIirTempMask) != 0U;
        c.shadow_iir_pressure = (r[0] & dsp_config::kShadowIirPressMask) != 0U;
        c.flush_forced_enable = (r[0] & dsp_config::kIirFlushForcedEnMask) != 0U;
        c.temperature = static_cast<IirFilter>(r[1] & dsp_iir::kTempMask);
        c.pressure = static_cast<IirFilter>((r[1] & dsp_iir::kPressMask) >> dsp_iir::kPressShift);
        return DriverResult<IirConfig>::success(c);
    }

    /// IIR registers are writable in standby only; previous mode is restored.
    DriverResult<void> SetIir(const IirConfig& c) noexcept {
        const auto cur = GetPowerMode();
        if (!cur.ok()) {
            return DriverResult<void>::failure(cur.error);
        }
        if (cur.value != PowerMode::Standby) {
            const auto r = SetPowerMode(PowerMode::Standby);
            if (!r.ok()) return r;
        }
        uint8_t r[2]{};
        if (!bus_.ReadRegs(reg::kDspConfig, r, 2)) {
            return DriverResult<void>::failure(DriverError::Bus);
        }
        r[0] = static_cast<uint8_t>((r[0] & ~(dsp_config::kShadowIirTempMask | dsp_config::kShadowIirPressMask |
                                              dsp_config::kIirFlushForcedEnMask)) |
                                    (c.shadow_iir_temperature ? dsp_config::kShadowIirTempMask : 0U) |
                                    (c.shadow_iir_pressure ? dsp_config::kShadowIirPressMask : 0U) |
                                    (c.flush_forced_enable ? dsp_config::kIirFlushForcedEnMask : 0U));
        r[1] = static_cast<uint8_t>((r[1] & ~(dsp_iir::kTempMask | dsp_iir::kPressMask)) |
                                    (static_cast<uint8_t>(c.temperature) & dsp_iir::kTempMask) |
                                    ((static_cast<uint8_t>(c.pressure) << dsp_iir::kPressShift) &
                                     dsp_iir::kPressMask));
        if (!bus_.WriteRegs(reg::kDspConfig, r, 2)) {
            return DriverResult<void>::failure(DriverError::Bus);
        }
        if (cur.value != PowerMode::Standby && cur.value != PowerMode::DeepStandby) {
            return SetPowerMode(cur.value);
        }
        return DriverResult<void>::success();
    }

    // ---------------------------------------------------------------- interrupts

    /// Disable sources, clear status, then write mode/polarity/drive/enable.
    DriverResult<void> ConfigureInterrupt(const InterruptConfig& c) noexcept {
        uint8_t cfg = 0;
        if (!bus_.ReadRegs(reg::kIntConfig, &cfg, 1)) {
            return DriverResult<void>::failure(DriverError::Bus);
        }
        if (!WriteReg(reg::kIntSource, 0x00)) {
            return DriverResult<void>::failure(DriverError::Bus);
        }
        uint8_t st = 0;
        (void)bus_.ReadRegs(reg::kIntStatus, &st, 1);
        cfg = static_cast<uint8_t>((cfg & ~(int_config::kModeMask | int_config::kPolMask | int_config::kOdMask |
                                            int_config::kEnableMask)) |
                                   (c.mode == IntMode::Latched ? int_config::kModeMask : 0U) |
                                   (c.polarity == IntPolarity::ActiveHigh ? int_config::kPolMask : 0U) |
                                   (c.drive == IntDrive::OpenDrain ? int_config::kOdMask : 0U) |
                                   (c.enable ? int_config::kEnableMask : 0U));
        return WriteReg(reg::kIntConfig, cfg) ? DriverResult<void>::success()
                                              : DriverResult<void>::failure(DriverError::Bus);
    }

    DriverResult<void> SelectInterruptSources(const InterruptSources& s) noexcept {
        const uint8_t v = static_cast<uint8_t>((s.data_ready ? int_source::kDataReady : 0U) |
                                               (s.fifo_full ? int_source::kFifoFull : 0U) |
                                               (s.fifo_threshold ? int_source::kFifoThreshold : 0U) |
                                               (s.pressure_out_of_range ? int_source::kPressureOor : 0U));
        return WriteReg(reg::kIntSource, v) ? DriverResult<void>::success()
                                            : DriverResult<void>::failure(DriverError::Bus);
    }

    /// INT_STATUS (read clears in pulsed mode).
    DriverResult<uint8_t> ReadInterruptStatus() noexcept {
        uint8_t st = 0;
        if (!bus_.ReadRegs(reg::kIntStatus, &st, 1)) {
            return DriverResult<uint8_t>::failure(DriverError::Bus);
        }
        return DriverResult<uint8_t>::success(st);
    }

    /// Convenience: INT_STATUS.drdy_data_reg.
    DriverResult<bool> DataReady() noexcept {
        const auto st = ReadInterruptStatus();
        if (!st.ok()) {
            return DriverResult<bool>::failure(st.error);
        }
        return DriverResult<bool>::success((st.value & int_status::kDataReady) != 0U);
    }

    // ---------------------------------------------------------------------- data

    /// Burst-read 0x1D..0x22 → temperature (1/65536 °C) and pressure (1/64 Pa).
    DriverResult<Sample> ReadSample(bool pressure_enabled = true) noexcept {
        uint8_t d[6]{};
        if (!bus_.ReadRegs(reg::kTempDataXlsb, d, 6)) {
            return DriverResult<Sample>::failure(DriverError::Bus);
        }
        return DriverResult<Sample>::success(DecodeSample(d, pressure_enabled));
    }

    /// Decode a 6-byte data-register burst (also used by tests).
    static Sample DecodeSample(const uint8_t d[6], bool pressure_enabled) noexcept {
        Sample s{};
        uint32_t t = (static_cast<uint32_t>(d[2]) << 16) | (static_cast<uint32_t>(d[1]) << 8) | d[0];
        if ((t & 0x800000U) != 0U) {
            t |= 0xFF000000U;
        }
        s.temperature_raw = static_cast<int32_t>(t);
        s.temperature_c = static_cast<float>(s.temperature_raw) / kTemperatureLsbPerC;
        if (pressure_enabled) {
            s.pressure_raw = (static_cast<uint32_t>(d[5]) << 16) | (static_cast<uint32_t>(d[4]) << 8) | d[3];
            s.pressure_pa = static_cast<float>(s.pressure_raw) / kPressureLsbPerPa;
        }
        return s;
    }

    // ----------------------------------------------------------------------- NVM

    /**
     * @brief Read one 16-bit NVM user word (0x20..0x22). Requires standby;
     *        previous mode is restored.
     */
    DriverResult<uint16_t> ReadNvm(uint8_t nvm_addr) noexcept {
        if (nvm_addr < nvm::kStartAddr || nvm_addr > nvm::kEndAddr) {
            return DriverResult<uint16_t>::failure(DriverError::NvmInvalidAddr);
        }
        const auto cur = GetPowerMode();
        if (!cur.ok()) {
            return DriverResult<uint16_t>::failure(cur.error);
        }
        if (cur.value != PowerMode::Standby) {
            const auto r = SetPowerMode(PowerMode::Standby);
            if (!r.ok()) return DriverResult<uint16_t>::failure(r.error);
        }
        DriverError err = NvmReadyCheck();
        if (err != DriverError::None) {
            return DriverResult<uint16_t>::failure(err);
        }
        if (!WriteReg(reg::kNvmAddr, static_cast<uint8_t>(nvm_addr & 0x3FU)) ||
            !WriteReg(reg::kCmd, cmd::kNvmFirst) || !WriteReg(reg::kCmd, cmd::kNvmReadEnable)) {
            return DriverResult<uint16_t>::failure(DriverError::Bus);
        }
        bus_.DelayUs(timing::kNvmReadyReadUs);
        err = NvmReadyCheck();
        if (err != DriverError::None) {
            return DriverResult<uint16_t>::failure(err);
        }
        uint8_t d[2]{};
        if (!bus_.ReadRegs(reg::kNvmDataLsb, d, 2)) {
            return DriverResult<uint16_t>::failure(DriverError::Bus);
        }
        if (cur.value != PowerMode::Standby) {
            (void)SetPowerMode(cur.value);
        }
        return DriverResult<uint16_t>::success(static_cast<uint16_t>((d[1] << 8) | d[0]));
    }

    // ---------------------------------------------------------------- raw access

    DriverResult<uint8_t> ReadRegister(uint8_t r) noexcept {
        uint8_t v = 0;
        if (!bus_.ReadRegs(r, &v, 1)) {
            return DriverResult<uint8_t>::failure(DriverError::Bus);
        }
        return DriverResult<uint8_t>::success(v);
    }

    DriverResult<void> WriteRegister(uint8_t r, uint8_t v) noexcept {
        return WriteReg(r, v) ? DriverResult<void>::success()
                              : DriverResult<void>::failure(DriverError::Bus);
    }

private:
    bool WriteReg(uint8_t r, uint8_t v) noexcept { return bus_.WriteRegs(r, &v, 1); }

    bool WritePowerModeBits(PowerMode mode, bool deep_disable) noexcept {
        uint8_t odr = 0;
        if (!bus_.ReadRegs(reg::kOdrConfig, &odr, 1)) {
            return false;
        }
        odr = static_cast<uint8_t>((odr & ~(odr_config::kPowerModeMask | odr_config::kDeepDisableMask)) |
                                   (static_cast<uint8_t>(mode) & odr_config::kPowerModeMask) |
                                   (deep_disable ? odr_config::kDeepDisableMask : 0U));
        return WriteReg(reg::kOdrConfig, odr);
    }

    /// Bosch API: deep standby is only entered when ODR ≤ 5 Hz, FIFO off and IIR bypassed.
    PowerMode DeepStandbyOrStandby() noexcept {
        uint8_t fifo_sel = 0;
        (void)bus_.ReadRegs(reg::kFifoSel, &fifo_sel, 1);
        const auto osr = GetOsrOdr();
        const auto iir = GetIir();
        if (osr.ok() && iir.ok() && (fifo_sel & 0x03U) == 0U &&
            static_cast<uint8_t>(osr.value.odr) > static_cast<uint8_t>(Odr::Hz5) &&
            iir.value.temperature == IirFilter::Bypass && iir.value.pressure == IirFilter::Bypass) {
            return PowerMode::DeepStandby;
        }
        return PowerMode::Standby;
    }

    DriverError NvmReadyCheck() noexcept {
        uint8_t st = 0;
        if (!bus_.ReadRegs(reg::kStatus, &st, 1)) {
            return DriverError::Bus;
        }
        if ((st & status::kNvmCmdErr) != 0U) {
            return DriverError::NvmCommandError;
        }
        if ((st & status::kNvmRdy) == 0U || (st & status::kNvmErr) != 0U) {
            return DriverError::NvmNotReady;
        }
        return DriverError::None;
    }

    BusT& bus_;
    uint8_t chip_id_{0};
};

}  // namespace bmp581
