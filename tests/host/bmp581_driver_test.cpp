// =============================================================================
// HF-BMP581 host tests — register-map fake bus: init, reset, power mode hop,
// OSR/ODR + IIR bit packing, interrupt config, sample decode, NVM read.
// =============================================================================
#include "bmp581.hpp"

#include <cstdio>
#include <cstring>
#include <vector>

namespace {

int g_failures = 0;

#define CHECK(cond)                                                                    \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                \
            ++g_failures;                                                              \
        }                                                                              \
    } while (0)

/// Register-map fake of the chip on its bus: writes land in regs[], reads come from regs[]; CMD is
/// intercepted to emulate soft reset and NVM read enable. It also models what the datasheet says the
/// bus does around a soft reset:
///  - I2C wiring (`spi_wired == false`): the write of 0xB6 to CMD is not acknowledged, so the
///    adapter reports it as failed (section 7.36), though the reset happens.
///  - SPI wiring: the chip starts in I2C mode and returns to it after a soft reset; the first SPI
///    transaction only switches it to SPI and its read data is invalid (section 5.1, Table 16).
///  - The chip does not answer until t_soft_res (2 ms, Table 4) after the reset command.
///  - INT_STATUS clears on read (section 7.21).
/// `Self` sets the adapter type the driver sees, so each variant can declare `kInterface` or not.
template <typename Self>
class FakeBusBase : public bmp581::BusInterface<Self> {
public:
    uint8_t regs[0x80]{};
    uint32_t delayed_us{0};
    std::vector<std::pair<uint8_t, uint8_t>> writes;
    bool fail_bus{false};
    uint8_t status_after_reset{0xFF};   // 0xFF = leave STATUS unchanged on soft reset
    uint8_t chip_id_after_reset{0xFF};  // 0xFF = leave CHIP_ID unchanged on soft reset
    bool por_after_reset{true};         // INT_STATUS.por set by the reset
    bool spi_wired{false};              // false: I2C bus; true: SPI bus (I2C mode until selected)
    bool spi_selected{false};           // SPI wiring: interface currently in SPI mode
    bool reads_fail_after_reset{false}; // every read after a soft reset reports a bus error
    uint32_t busy_us{0};                // time left until the chip answers again after a reset
    int reset_count{0};
    int invalid_reads{0};               // reads answered with invalid data (SPI not selected)
    int reads_after_reset{0};

    FakeBusBase() {
        regs[bmp581::reg::kChipId] = bmp581::chip_id::kPrimary;
        regs[bmp581::reg::kStatus] = bmp581::status::kNvmRdy | bmp581::status::kCoreRdy;
        regs[bmp581::reg::kOdrConfig] = bmp581::odr_config::kDeepDisableMask;  // standby, deep disabled
        regs[bmp581::reg::kIntStatus] = 0;
        regs[bmp581::reg::kNvmDataLsb] = 0x34;
        regs[bmp581::reg::kNvmDataMsb] = 0x12;
    }

    bool WriteRegs(uint8_t reg, const uint8_t* data, std::size_t len) noexcept {
        if (fail_bus || busy_us > 0) return false;
        if (spi_wired && !spi_selected) {
            spi_selected = true;  // this transaction selected SPI; its content is lost
            return true;
        }
        bool acked = true;
        for (std::size_t i = 0; i < len; ++i) {
            const uint8_t r = static_cast<uint8_t>(reg + i);
            writes.emplace_back(r, data[i]);
            if (r == bmp581::reg::kCmd) {
                if (data[i] == bmp581::cmd::kSoftReset) {
                    Reset();
                    acked = spi_wired;  // I2C: no ACK for the reset command
                }
                continue;
            }
            regs[r] = data[i];
        }
        return acked;
    }
    bool ReadRegs(uint8_t reg, uint8_t* out, std::size_t len) noexcept {
        if (fail_bus || busy_us > 0) return false;
        if (reset_count > 0) {
            ++reads_after_reset;
            if (reads_fail_after_reset) return false;
        }
        if (spi_wired && !spi_selected) {
            spi_selected = true;
            ++invalid_reads;
            std::memset(out, 0x00, len);  // invalid data
            return true;
        }
        std::memcpy(out, &regs[reg], len);
        if (reg <= bmp581::reg::kIntStatus && reg + len > bmp581::reg::kIntStatus) {
            regs[bmp581::reg::kIntStatus] = 0;  // clear-on-read
        }
        return true;
    }
    void DelayUs(uint32_t us) noexcept {
        delayed_us += us;
        busy_us = us >= busy_us ? 0 : busy_us - us;
    }
    bool EnsureInitialized() noexcept { return true; }

private:
    void Reset() noexcept {
        ++reset_count;
        busy_us = bmp581::timing::kSoftResetUs;
        spi_selected = false;
        regs[bmp581::reg::kIntStatus] = por_after_reset ? bmp581::int_status::kPorSoftResetComplete : 0;
        regs[bmp581::reg::kOdrConfig] = bmp581::odr_config::kDeepDisableMask;
        if (status_after_reset != 0xFF) {
            regs[bmp581::reg::kStatus] = status_after_reset;
        }
        if (chip_id_after_reset != 0xFF) {
            regs[bmp581::reg::kChipId] = chip_id_after_reset;
        }
    }
};

/// Adapter that does not declare its interface (the original contract).
class FakeBus : public FakeBusBase<FakeBus> {};

/// Adapter that declares I2C.
class FakeI2c : public FakeBusBase<FakeI2c> {
public:
    static constexpr bmp581::Interface kInterface = bmp581::Interface::I2c;
};

/// Adapter that declares SPI; the chip behind it is wired for SPI.
class FakeSpi : public FakeBusBase<FakeSpi> {
public:
    static constexpr bmp581::Interface kInterface = bmp581::Interface::Spi;
    FakeSpi() { spi_wired = true; }
};

void TestInitAndReset() {
    FakeBus bus;
    bmp581::Driver<FakeBus> dev(bus);
    CHECK(dev.Init().ok());
    CHECK(dev.ChipId() == 0x50);
    CHECK(dev.SoftReset().ok());
    CHECK(bus.delayed_us >= bmp581::timing::kSoftResetUs);

    FakeBus bad;
    bad.regs[bmp581::reg::kChipId] = 0x12;
    bmp581::Driver<FakeBus> dbad(bad);
    CHECK(dbad.Init().error == bmp581::DriverError::InvalidChipId);

    FakeBus nvm;
    nvm.regs[bmp581::reg::kStatus] = bmp581::status::kNvmErr | bmp581::status::kNvmRdy;
    bmp581::Driver<FakeBus> dnvm(nvm);
    CHECK(dnvm.Init().error == bmp581::DriverError::NvmNotReady);

    // STATUS == 0 with a valid chip id → Init soft-resets once and re-checks (bench-observed).
    FakeBus cold;
    cold.regs[bmp581::reg::kStatus] = 0x00;
    cold.status_after_reset = bmp581::status::kNvmRdy | bmp581::status::kCoreRdy;
    bmp581::Driver<FakeBus> dcold(cold);
    CHECK(dcold.Init().ok());
    CHECK(cold.delayed_us >= bmp581::timing::kSoftResetUs);
    FakeBus dead;
    dead.regs[bmp581::reg::kStatus] = 0x00;  // stays 0 after reset
    bmp581::Driver<FakeBus> ddead(dead);
    CHECK(ddead.Init().error == bmp581::DriverError::NvmNotReady);

    FakeBus off;
    off.fail_bus = true;
    bmp581::Driver<FakeBus> doff(off);
    CHECK(doff.Init().error == bmp581::DriverError::Bus);
}

// Soft reset over I2C: the chip does not acknowledge the write of 0xB6 to CMD (section 7.36), so
// the adapter reports that one write as failed. SoftReset() must accept it, wait t_soft_res and
// verify the reset by reading CHIP_ID, STATUS and INT_STATUS (section 4.3.9).
template <typename Bus>
void CheckSoftResetOverI2c(int expected_reads) {
    Bus bus;
    bmp581::Driver<Bus> dev(bus);
    CHECK(dev.Init().ok());
    const uint32_t before = bus.delayed_us;
    const auto r = dev.SoftReset();
    CHECK(r.ok());
    CHECK(r.error == bmp581::DriverError::None);
    CHECK(bus.reset_count == 1);
    CHECK(bus.delayed_us - before >= bmp581::timing::kSoftResetUs);
    CHECK(bus.reads_after_reset == expected_reads);
    CHECK(bus.invalid_reads == 0);
    CHECK(bus.regs[bmp581::reg::kIntStatus] == 0);  // por was read (and cleared)
}

void TestSoftResetI2cNack() {
    CheckSoftResetOverI2c<FakeI2c>(3);  // CHIP_ID, STATUS, INT_STATUS
    CheckSoftResetOverI2c<FakeBus>(4);  // undeclared interface: plus the SPI-selecting dummy read
}

// The reset is verified per section 4.3.9: CHIP_ID not 0, STATUS nvm_rdy = 1 and nvm_err = 0,
// INT_STATUS por = 1.
void TestSoftResetVerifies() {
    {
        FakeI2c bus;
        bus.chip_id_after_reset = 0x00;
        bmp581::Driver<FakeI2c> dev(bus);
        CHECK(dev.SoftReset().error == bmp581::DriverError::InvalidChipId);
    }
    {
        FakeI2c bus;
        bus.status_after_reset = bmp581::status::kCoreRdy;  // nvm_rdy = 0
        bmp581::Driver<FakeI2c> dev(bus);
        CHECK(dev.SoftReset().error == bmp581::DriverError::NvmNotReady);
    }
    {
        FakeI2c bus;
        bus.status_after_reset = bmp581::status::kNvmRdy | bmp581::status::kNvmErr;
        bmp581::Driver<FakeI2c> dev(bus);
        CHECK(dev.SoftReset().error == bmp581::DriverError::NvmNotReady);
    }
    {
        FakeI2c bus;
        bus.por_after_reset = false;
        bmp581::Driver<FakeI2c> dev(bus);
        CHECK(dev.SoftReset().error == bmp581::DriverError::PorSoftReset);
    }
}

// Only the missing ACK of the reset write is tolerated: a failed read after the reset, a dead
// bus, and a failed reset write on SPI (which has no ACK to miss) are all reported as Bus.
void TestSoftResetBusErrors() {
    {
        FakeI2c bus;
        bus.reads_fail_after_reset = true;
        bmp581::Driver<FakeI2c> dev(bus);
        CHECK(dev.SoftReset().error == bmp581::DriverError::Bus);
    }
    {
        FakeBus bus;
        bus.reads_fail_after_reset = true;
        bmp581::Driver<FakeBus> dev(bus);
        CHECK(dev.SoftReset().error == bmp581::DriverError::Bus);
    }
    {
        FakeI2c bus;
        bus.fail_bus = true;
        bmp581::Driver<FakeI2c> dev(bus);
        CHECK(dev.SoftReset().error == bmp581::DriverError::Bus);
    }
    {
        FakeSpi bus;
        bmp581::Driver<FakeSpi> dev(bus);
        CHECK(dev.Init().ok());
        bus.fail_bus = true;
        CHECK(dev.SoftReset().error == bmp581::DriverError::Bus);
        CHECK(bus.reset_count == 0);
    }
    {
        FakeSpi bus;
        bus.reads_fail_after_reset = true;
        bmp581::Driver<FakeSpi> dev(bus);
        CHECK(dev.Init().ok());
        CHECK(dev.SoftReset().error == bmp581::DriverError::Bus);
    }
}

// SPI: the chip is in I2C mode after power-up and after a soft reset; one dummy read selects SPI
// and returns invalid data (section 5.1, Table 16). The driver does the dummy read itself.
template <typename Bus>
void CheckSpiReentry() {
    Bus bus;
    bus.spi_wired = true;
    bmp581::Driver<Bus> dev(bus);
    CHECK(dev.Init().ok());  // first access after power-up selects SPI
    CHECK(dev.ChipId() == bmp581::chip_id::kPrimary);
    CHECK(bus.invalid_reads == 1);
    CHECK(dev.SoftReset().ok());
    CHECK(bus.reset_count == 1);
    CHECK(bus.invalid_reads == 2);  // the dummy read after the reset
    CHECK(bus.spi_selected);
    CHECK(bus.regs[bmp581::reg::kIntStatus] == 0);  // por was read from the chip, not the dummy
    const auto id = dev.ReadRegister(bmp581::reg::kChipId);
    CHECK(id.ok() && id.value == bmp581::chip_id::kPrimary);
    CHECK(dev.Configure(bmp581::BarometricReferencePreset()).ok());
    CHECK(dev.GetPowerMode().value == bmp581::PowerMode::Normal);
}

void TestSoftResetSpiReentry() {
    CheckSpiReentry<FakeSpi>();
    CheckSpiReentry<FakeBus>();  // undeclared interface on an SPI bus
}

// Init()'s recovery reset for a part whose NVM boot did not finish must run over I2C, where the
// reset write is not acknowledged.
void TestInitColdNvmOverI2c() {
    FakeI2c bus;
    bus.regs[bmp581::reg::kStatus] = 0x00;
    bus.status_after_reset = bmp581::status::kNvmRdy | bmp581::status::kCoreRdy;
    bmp581::Driver<FakeI2c> dev(bus);
    CHECK(dev.Init().ok());
    CHECK(bus.reset_count == 1);
    CHECK(bus.delayed_us >= bmp581::timing::kSoftResetUs);
}

void TestPowerModes() {
    FakeBus bus;
    bmp581::Driver<FakeBus> dev(bus);
    CHECK(dev.GetPowerMode().value == bmp581::PowerMode::Standby);
    CHECK(dev.SetPowerMode(bmp581::PowerMode::Normal).ok());
    CHECK((bus.regs[bmp581::reg::kOdrConfig] & 0x03) == 0x01);
    CHECK((bus.regs[bmp581::reg::kOdrConfig] & 0x80) != 0);
    CHECK(dev.GetPowerMode().value == bmp581::PowerMode::Normal);
    // Normal → Continuous must hop through standby with t_standby
    const uint32_t before = bus.delayed_us;
    CHECK(dev.SetPowerMode(bmp581::PowerMode::Continuous).ok());
    CHECK(bus.delayed_us - before >= bmp581::timing::kStandbyUs);
    CHECK((bus.regs[bmp581::reg::kOdrConfig] & 0x03) == 0x03);
    // Deep standby clears deep_dis
    CHECK(dev.SetPowerMode(bmp581::PowerMode::DeepStandby).ok());
    CHECK((bus.regs[bmp581::reg::kOdrConfig] & 0x83) == 0x00);
}

void TestOsrOdrIir() {
    FakeBus bus;
    bmp581::Driver<FakeBus> dev(bus);
    bus.regs[bmp581::reg::kOdrConfig] = 0x81;  // normal, deep disabled
    bmp581::OsrOdrConfig c{};
    c.osr_pressure = bmp581::Oversampling::X8;
    c.osr_temperature = bmp581::Oversampling::X2;
    c.pressure_enable = true;
    c.odr = bmp581::Odr::Hz100_2;
    CHECK(dev.SetOsrOdr(c).ok());
    CHECK(bus.regs[bmp581::reg::kOsrConfig] == static_cast<uint8_t>(0x40 | (3 << 3) | 1));
    CHECK(bus.regs[bmp581::reg::kOdrConfig] == static_cast<uint8_t>(0x81 | (0x0A << 2)));
    const auto back = dev.GetOsrOdr();
    CHECK(back.ok() && back.value.odr == bmp581::Odr::Hz100_2 &&
          back.value.osr_pressure == bmp581::Oversampling::X8 && back.value.pressure_enable);

    bmp581::IirConfig iir{};
    iir.pressure = bmp581::IirFilter::Coeff3;
    iir.temperature = bmp581::IirFilter::Coeff1;
    iir.shadow_iir_pressure = true;
    iir.shadow_iir_temperature = false;
    CHECK(dev.SetIir(iir).ok());
    CHECK(bus.regs[bmp581::reg::kDspIir] == static_cast<uint8_t>((2 << 3) | 1));
    CHECK((bus.regs[bmp581::reg::kDspConfig] & 0x28) == 0x20);
    // Mode restored to Normal after the standby-only IIR write
    CHECK(dev.GetPowerMode().value == bmp581::PowerMode::Normal);
    const auto rd = dev.GetIir();
    CHECK(rd.ok() && rd.value.pressure == bmp581::IirFilter::Coeff3 && rd.value.temperature == bmp581::IirFilter::Coeff1);

    bus.regs[bmp581::reg::kOsrEff] = 0x80 | (3 << 3) | 1;
    const auto eff = dev.GetEffectiveOsr();
    CHECK(eff.ok() && eff.value.odr_valid && eff.value.osr_pressure == bmp581::Oversampling::X8);
}

void TestInterruptsAndSample() {
    FakeBus bus;
    bmp581::Driver<FakeBus> dev(bus);
    bmp581::InterruptConfig ic{};
    ic.mode = bmp581::IntMode::Latched;
    ic.polarity = bmp581::IntPolarity::ActiveHigh;
    ic.drive = bmp581::IntDrive::OpenDrain;
    ic.enable = true;
    CHECK(dev.ConfigureInterrupt(ic).ok());
    CHECK((bus.regs[bmp581::reg::kIntConfig] & 0x0F) == 0x0F);
    bmp581::InterruptSources src{};
    src.data_ready = true;
    CHECK(dev.SelectInterruptSources(src).ok());
    CHECK(bus.regs[bmp581::reg::kIntSource] == 0x01);

    bus.regs[bmp581::reg::kIntStatus] = bmp581::int_status::kDataReady;
    CHECK(dev.DataReady().value);

    // 101325 Pa → raw 6484800 = 0x62F340 ; 25.5 °C → raw 1671168 = 0x198000
    const uint8_t d[6] = {0x00, 0x80, 0x19, 0x40, 0xF3, 0x62};
    const auto s = bmp581::Driver<FakeBus>::DecodeSample(d, true);
    CHECK(s.pressure_raw == 6484800U);
    CHECK(s.pressure_pa > 101324.9f && s.pressure_pa < 101325.1f);
    CHECK(s.temperature_c > 25.49f && s.temperature_c < 25.51f);
    // Negative temperature: −10 °C → −655360 = 0xF60000 (24-bit two's complement)
    const uint8_t n[6] = {0x00, 0x00, 0xF6, 0, 0, 0};
    const auto sn = bmp581::Driver<FakeBus>::DecodeSample(n, false);
    CHECK(sn.temperature_c > -10.01f && sn.temperature_c < -9.99f);
    CHECK(sn.pressure_pa == 0.0f);

    std::memcpy(&bus.regs[bmp581::reg::kTempDataXlsb], d, 6);
    const auto rs = dev.ReadSample();
    CHECK(rs.ok() && rs.value.pressure_raw == 6484800U);
}

// INT_CONFIG (0x14) layout, datasheet section 7.5: int_mode 0, int_pol 1, int_od 2, int_en 3,
// pad_int_drv 7:4. The fields must tile the register with no gap or overlap.
void TestIntConfigFields() {
    namespace ic = bmp581::int_config;
    CHECK(ic::kPadDriveMask == 0xF0);
    CHECK((ic::kPadDriveMask >> ic::kPadDriveShift) == 0x0F);
    CHECK((ic::kModeMask | ic::kPolMask | ic::kOdMask | ic::kEnableMask | ic::kPadDriveMask) == 0xFF);
    CHECK((ic::kPadDriveMask & (ic::kModeMask | ic::kPolMask | ic::kOdMask | ic::kEnableMask)) == 0);

    // ConfigureInterrupt() leaves the whole pad_int_drv field as it found it.
    FakeBus bus;
    bus.regs[bmp581::reg::kIntConfig] = 0xF0;
    bmp581::Driver<FakeBus> dev(bus);
    bmp581::InterruptConfig cfg{};
    cfg.enable = true;
    CHECK(dev.ConfigureInterrupt(cfg).ok());
    CHECK((bus.regs[bmp581::reg::kIntConfig] & ic::kPadDriveMask) == 0xF0);
}

void TestNvm() {
    FakeBus bus;
    bmp581::Driver<FakeBus> dev(bus);
    bus.regs[bmp581::reg::kOdrConfig] = 0x81;  // normal
    const auto v = dev.ReadNvm(0x21);
    CHECK(v.ok() && v.value == 0x1234);
    CHECK(dev.GetPowerMode().value == bmp581::PowerMode::Normal);
    CHECK(dev.ReadNvm(0x23).error == bmp581::DriverError::NvmInvalidAddr);
    bool saw_first = false, saw_enable = false;
    for (const auto& w : bus.writes) {
        if (w.first == bmp581::reg::kCmd && w.second == bmp581::cmd::kNvmFirst) saw_first = true;
        if (w.first == bmp581::reg::kCmd && w.second == bmp581::cmd::kNvmReadEnable) saw_enable = true;
    }
    CHECK(saw_first && saw_enable);
}

void TestPreset() {
    const auto p = bmp581::BarometricReferencePreset();
    CHECK(p.osr_odr.odr == bmp581::Odr::Hz100_2);
    CHECK(bmp581::OdrHz(p.osr_odr.odr) > 100.0f);
    FakeBus bus;
    bmp581::Driver<FakeBus> dev(bus);
    CHECK(dev.Configure(p).ok());
    CHECK(dev.GetPowerMode().value == bmp581::PowerMode::Normal);
}

}  // namespace

int main() {
    TestInitAndReset();
    TestSoftResetI2cNack();
    TestSoftResetVerifies();
    TestSoftResetBusErrors();
    TestSoftResetSpiReentry();
    TestInitColdNvmOverI2c();
    TestPowerModes();
    TestOsrOdrIir();
    TestInterruptsAndSample();
    TestIntConfigFields();
    TestNvm();
    TestPreset();
    if (g_failures == 0) {
        std::printf("hf-bmp581 host tests: all passed (%s)\n", bmp581::GetDriverVersion());
        return 0;
    }
    std::printf("hf-bmp581 host tests: %d failure(s)\n", g_failures);
    return 1;
}
