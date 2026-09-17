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

/// Register-map fake: writes land in regs[], reads come from regs[]; CMD is
/// intercepted to emulate soft reset and NVM read enable.
class FakeBus : public bmp581::BusInterface<FakeBus> {
public:
    uint8_t regs[0x80]{};
    uint32_t delayed_us{0};
    std::vector<std::pair<uint8_t, uint8_t>> writes;
    bool fail_bus{false};
    uint8_t status_after_reset{0xFF};  // 0xFF = leave STATUS unchanged on soft reset

    FakeBus() {
        regs[bmp581::reg::kChipId] = bmp581::chip_id::kPrimary;
        regs[bmp581::reg::kStatus] = bmp581::status::kNvmRdy | bmp581::status::kCoreRdy;
        regs[bmp581::reg::kOdrConfig] = bmp581::odr_config::kDeepDisableMask;  // standby, deep disabled
        regs[bmp581::reg::kIntStatus] = 0;
        regs[bmp581::reg::kNvmDataLsb] = 0x34;
        regs[bmp581::reg::kNvmDataMsb] = 0x12;
    }

    bool WriteRegs(uint8_t reg, const uint8_t* data, std::size_t len) noexcept {
        if (fail_bus) return false;
        for (std::size_t i = 0; i < len; ++i) {
            const uint8_t r = static_cast<uint8_t>(reg + i);
            writes.emplace_back(r, data[i]);
            if (r == bmp581::reg::kCmd) {
                if (data[i] == bmp581::cmd::kSoftReset) {
                    regs[bmp581::reg::kIntStatus] |= bmp581::int_status::kPorSoftResetComplete;
                    regs[bmp581::reg::kOdrConfig] = bmp581::odr_config::kDeepDisableMask;
                    if (status_after_reset != 0xFF) {
                        regs[bmp581::reg::kStatus] = status_after_reset;
                    }
                }
                continue;
            }
            regs[r] = data[i];
        }
        return true;
    }
    bool ReadRegs(uint8_t reg, uint8_t* out, std::size_t len) noexcept {
        if (fail_bus) return false;
        std::memcpy(out, &regs[reg], len);
        return true;
    }
    void DelayUs(uint32_t us) noexcept { delayed_us += us; }
    bool EnsureInitialized() noexcept { return true; }
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
    TestPowerModes();
    TestOsrOdrIir();
    TestInterruptsAndSample();
    TestNvm();
    TestPreset();
    if (g_failures == 0) {
        std::printf("hf-bmp581 host tests: all passed (%s)\n", bmp581::GetDriverVersion());
        return 0;
    }
    std::printf("hf-bmp581 host tests: %d failure(s)\n", g_failures);
    return 1;
}
