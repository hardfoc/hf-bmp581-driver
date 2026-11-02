/**
 * @file api_tasks.cpp
 * @brief The API guide's snippets, one function per task, compiled by the host build (never run).
 */
#include "my_i2c.hpp"

#include <cstdio>

using Driver = bmp581::Driver<MyI2c>;

// --8<-- [start:init]
bool bring_up(Driver& dev) {
  const auto init = dev.Init();             // chip id 0x50 / 0x51, STATUS.nvm_rdy, no NVM error
  if (!init) {
    std::printf("init: %s\n", bmp581::ToString(init.error).data());
    return false;
  }
  const auto reset = dev.SoftReset();       // CMD 0xB6, 2 ms, INT_STATUS.por checked
  if (!reset) {
    std::printf("soft reset: %s\n", bmp581::ToString(reset.error).data());
    return false;
  }
  std::printf("BMP581 chip id 0x%02X, driver %s\n", dev.ChipId(), bmp581::GetDriverVersion());
  return true;
}
// --8<-- [end:init]

// --8<-- [start:configure]
bool configure_low_noise(Driver& dev) {
  bmp581::Config cfg{};
  cfg.osr_odr.osr_pressure = bmp581::Oversampling::X64;
  cfg.osr_odr.osr_temperature = bmp581::Oversampling::X4;
  cfg.osr_odr.odr = bmp581::Odr::Hz10;      // datasheet table 7: at most 20 Hz with these OSRs
  cfg.iir.pressure = bmp581::IirFilter::Coeff7;
  cfg.power_mode = bmp581::PowerMode::Normal;
  if (!dev.Configure(cfg)) {                // standby, OSR / ODR, IIR, then the power mode
    return false;
  }
  const auto eff = dev.GetEffectiveOsr();   // the chip falls back to default OSRs if the ODR is too fast
  return eff && eff.value.odr_valid;
}
// --8<-- [end:configure]

// --8<-- [start:read]
bool read_when_ready(Driver& dev, float& pressure_pa, float& temperature_c) {
  const auto ready = dev.DataReady();       // reads INT_STATUS, which clears it
  if (!ready || !ready.value) {
    return false;                           // bus error, or no new sample yet
  }
  const auto s = dev.ReadSample();          // one 6-byte burst from 0x1D
  if (!s) {
    return false;
  }
  pressure_pa = s.value.pressure_pa;        // raw / 64
  temperature_c = s.value.temperature_c;    // raw / 65536
  return true;
}
// --8<-- [end:read]

// --8<-- [start:forced]
bool one_shot(Driver& dev, MyI2c& bus, bmp581::Sample& out) {
  if (!dev.SetPowerMode(bmp581::PowerMode::Forced)) {   // one conversion, then back to standby
    return false;
  }
  bus.DelayUs(bmp581::timing::kStartupUs + 6000);       // start-up plus OSR x8 / x1 conversion time
  const auto s = dev.ReadSample();
  if (!s) {
    return false;
  }
  out = s.value;
  return true;
}
// --8<-- [end:forced]

// --8<-- [start:interrupt]
bool enable_data_ready_pin(Driver& dev) {
  bmp581::InterruptConfig pin{};
  pin.mode = bmp581::IntMode::Pulsed;          // about 105 us pulse per sample
  pin.polarity = bmp581::IntPolarity::ActiveHigh;
  pin.drive = bmp581::IntDrive::PushPull;
  pin.enable = true;
  if (!dev.ConfigureInterrupt(pin)) {          // disables sources and clears INT_STATUS first
    return false;
  }
  bmp581::InterruptSources src{};
  src.data_ready = true;
  return dev.SelectInterruptSources(src).ok();
}
// --8<-- [end:interrupt]

// --8<-- [start:nvm]
bool read_user_words(Driver& dev, uint16_t (&words)[3]) {
  for (uint8_t i = 0; i < 3; ++i) {
    const auto w = dev.ReadNvm(static_cast<uint8_t>(bmp581::nvm::kStartAddr + i));  // rows 0x20..0x22
    if (!w) {
      return false;
    }
    words[i] = w.value;
  }
  return true;
}
// --8<-- [end:nvm]

// --8<-- [start:raw]
bool read_revision(Driver& dev, uint8_t& rev) {
  const auto r = dev.ReadRegister(bmp581::reg::kRevId);   // 0x32 on current silicon
  if (!r) {
    return false;
  }
  rev = r.value;
  return true;
}
// --8<-- [end:raw]

// --8<-- [start:errors]
const char* advice(bmp581::DriverError e) {
  switch (e) {
    case bmp581::DriverError::None:            return "ok";
    case bmp581::DriverError::NotInitialized:  return "EnsureInitialized() failed: bring the bus up";
    case bmp581::DriverError::Bus:             return "a transfer failed: wiring, address, pull-ups";
    case bmp581::DriverError::InvalidChipId:   return "not a BMP581, or the first SPI read";
    case bmp581::DriverError::NvmNotReady:     return "NVM boot not finished or NVM error";
    case bmp581::DriverError::PorSoftReset:    return "reset not confirmed by INT_STATUS.por";
    case bmp581::DriverError::NvmInvalidAddr:  return "NVM user rows are 0x20..0x22";
    case bmp581::DriverError::NvmCommandError: return "NVM command rejected (chip not in standby)";
    case bmp581::DriverError::InvalidParameter:
    case bmp581::DriverError::InvalidPowerMode: return "invalid argument";
  }
  return "unknown";
}
// --8<-- [end:errors]
