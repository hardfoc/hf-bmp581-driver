/**
 * @file quickstart.cpp
 * @brief The Quickstart page's example, compiled by the host build (never run).
 */
#include "my_i2c.hpp"

// --8<-- [start:quickstart]
#include "bmp581.hpp"

#include <cstdio>

int read_pressure_once(MyI2c& bus) {
  bmp581::Driver<MyI2c> dev(bus);

  if (const auto r = dev.Init(); !r) {                       // 1. chip id and NVM ready
    std::printf("init: %s\n", bmp581::ToString(r.error).data());
    return -1;                                               // wiring, address or supply
  }
  if (!dev.Configure(bmp581::BarometricReferencePreset())) { // 2. 100 Hz, OSR x8, IIR 3, normal mode
    return -1;
  }
  for (int i = 0; i < 50; ++i) {                             // 3. wait for the first conversion
    const auto ready = dev.DataReady();
    if (!ready || ready.value) break;
    bus.DelayUs(1000);
  }
  const auto s = dev.ReadSample();                           // 4. pressure (Pa) and temperature (C)
  if (!s) return -1;
  std::printf("%.1f Pa  %.2f C\n", static_cast<double>(s.value.pressure_pa),
              static_cast<double>(s.value.temperature_c));
  return 0;
}
// --8<-- [end:quickstart]
