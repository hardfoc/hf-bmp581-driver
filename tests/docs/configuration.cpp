/**
 * @file configuration.cpp
 * @brief The Configuration page's snippets, compiled by the host build (never run).
 */
#include "my_i2c.hpp"

// --8<-- [start:preset]
bmp581::DriverResult<void> use_preset(bmp581::Driver<MyI2c>& dev) {
  return dev.Configure(bmp581::BarometricReferencePreset());   // 100.3 Hz, OSR p x8 / t x1, IIR 3, normal
}
// --8<-- [end:preset]

// --8<-- [start:pieces]
bmp581::DriverResult<void> configure_in_pieces(bmp581::Driver<MyI2c>& dev) {
  bmp581::OsrOdrConfig osr{};
  osr.osr_pressure = bmp581::Oversampling::X16;
  osr.osr_temperature = bmp581::Oversampling::X1;
  osr.pressure_enable = true;
  osr.odr = bmp581::Odr::Hz50;
  if (auto r = dev.SetOsrOdr(osr); !r) {             // keeps the power-mode bits
    return r;
  }

  bmp581::IirConfig iir{};
  iir.pressure = bmp581::IirFilter::Coeff15;
  iir.temperature = bmp581::IirFilter::Bypass;
  if (auto r = dev.SetIir(iir); !r) {                // hops through standby and back
    return r;
  }
  return dev.SetPowerMode(bmp581::PowerMode::Normal);
}
// --8<-- [end:pieces]
