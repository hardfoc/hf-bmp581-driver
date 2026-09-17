/**
 * @file bmp581_types.hpp
 * @brief BMP581 driver types: results, configuration, samples.
 *
 * @copyright Copyright (c) 2026 HardFOC. All rights reserved.
 */
#pragma once

#include "bmp581_registers.hpp"

#include <cstdint>
#include <string_view>

namespace bmp581 {

/// Driver-level error codes (stable for logging / telemetry).
enum class DriverError : uint8_t {
    None = 0,
    InvalidParameter,
    NotInitialized,   ///< transport `EnsureInitialized()` failed
    Bus,              ///< register read/write failed
    InvalidChipId,    ///< CHIP_ID not 0x50 / 0x51
    NvmNotReady,      ///< STATUS.nvm_rdy clear or nvm_err set
    PorSoftReset,     ///< INT_STATUS.por_softreset_complete not set after reset
    InvalidPowerMode,
    NvmInvalidAddr,   ///< outside 0x20..0x22
    NvmCommandError,  ///< STATUS.nvm_cmd_err set
};

constexpr std::string_view ToString(DriverError e) noexcept {
    switch (e) {
        case DriverError::None:             return "None";
        case DriverError::InvalidParameter: return "InvalidParameter";
        case DriverError::NotInitialized:   return "NotInitialized";
        case DriverError::Bus:              return "Bus";
        case DriverError::InvalidChipId:    return "InvalidChipId";
        case DriverError::NvmNotReady:      return "NvmNotReady";
        case DriverError::PorSoftReset:     return "PorSoftReset";
        case DriverError::InvalidPowerMode: return "InvalidPowerMode";
        case DriverError::NvmInvalidAddr:   return "NvmInvalidAddr";
        case DriverError::NvmCommandError:  return "NvmCommandError";
    }
    return "?";
}

template <typename T>
struct DriverResult {
    T value{};
    DriverError error{DriverError::None};

    constexpr bool ok() const noexcept { return error == DriverError::None; }
    constexpr explicit operator bool() const noexcept { return ok(); }

    static constexpr DriverResult success(T v) noexcept { return {v, DriverError::None}; }
    static constexpr DriverResult failure(DriverError e) noexcept { return {T{}, e}; }
};

template <>
struct DriverResult<void> {
    DriverError error{DriverError::None};
    constexpr bool ok() const noexcept { return error == DriverError::None; }
    constexpr explicit operator bool() const noexcept { return ok(); }
    static constexpr DriverResult success() noexcept { return {DriverError::None}; }
    static constexpr DriverResult failure(DriverError e) noexcept { return {e}; }
};

constexpr std::string_view ToString(PowerMode m) noexcept {
    switch (m) {
        case PowerMode::Standby:     return "Standby";
        case PowerMode::Normal:      return "Normal";
        case PowerMode::Forced:      return "Forced";
        case PowerMode::Continuous:  return "Continuous";
        case PowerMode::DeepStandby: return "DeepStandby";
    }
    return "?";
}

/// Oversampling / ODR / pressure-enable (OSR_CONFIG + ODR_CONFIG).
struct OsrOdrConfig {
    Oversampling osr_temperature{Oversampling::X1};
    Oversampling osr_pressure{Oversampling::X1};
    bool pressure_enable{true};
    Odr odr{Odr::Hz50};
};

/// IIR configuration (DSP_CONFIG + DSP_IIR). Writable in standby only.
struct IirConfig {
    IirFilter temperature{IirFilter::Bypass};
    IirFilter pressure{IirFilter::Bypass};
    bool shadow_iir_temperature{true};  ///< data registers show IIR-filtered temperature
    bool shadow_iir_pressure{true};     ///< data registers show IIR-filtered pressure
    bool flush_forced_enable{false};    ///< flush IIR on each forced-mode conversion
};

/// Effective oversampling and ODR validity (OSR_EFF).
struct EffectiveOsr {
    Oversampling osr_temperature{Oversampling::X1};
    Oversampling osr_pressure{Oversampling::X1};
    bool odr_valid{true};  ///< false when the OSR combination cannot meet the ODR
};

/// Interrupt pin configuration (INT_CONFIG).
struct InterruptConfig {
    IntMode mode{IntMode::Pulsed};
    IntPolarity polarity{IntPolarity::ActiveHigh};
    IntDrive drive{IntDrive::PushPull};
    bool enable{false};
};

/// Interrupt sources (INT_SOURCE).
struct InterruptSources {
    bool data_ready{false};
    bool fifo_full{false};
    bool fifo_threshold{false};
    bool pressure_out_of_range{false};
};

/// One pressure / temperature sample.
struct Sample {
    uint32_t pressure_raw{0};   ///< 24-bit unsigned, 1/64 Pa
    int32_t temperature_raw{0}; ///< 24-bit two's complement, 1/65536 °C
    float pressure_pa{0.0f};
    float temperature_c{0.0f};
};

/// Convenience: one-call configuration for `Driver::Configure()`.
struct Config {
    OsrOdrConfig osr_odr{};
    IirConfig iir{};
    PowerMode power_mode{PowerMode::Normal};
};

/**
 * @brief Barometric preset for a ~100 Hz reference channel.
 * OSR ×8 pressure / ×1 temperature keeps the ODR valid at 100 Hz on the BMP581;
 * IIR coeff 3 trims the last few Pa of noise without adding meaningful lag.
 */
constexpr Config BarometricReferencePreset() noexcept {
    Config c{};
    c.osr_odr.osr_pressure = Oversampling::X8;
    c.osr_odr.osr_temperature = Oversampling::X1;
    c.osr_odr.pressure_enable = true;
    c.osr_odr.odr = Odr::Hz100_2;
    c.iir.pressure = IirFilter::Coeff3;
    c.iir.temperature = IirFilter::Coeff3;
    c.power_mode = PowerMode::Normal;
    return c;
}

}  // namespace bmp581
