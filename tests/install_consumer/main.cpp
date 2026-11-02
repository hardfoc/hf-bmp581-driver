/**
 * @file main.cpp
 * @brief Compiles every public header from an installed copy of the driver.
 */
#include "bmp581.hpp"
#include "bmp581_bus_interface.hpp"
#include "bmp581_driver.hpp"
#include "bmp581_registers.hpp"
#include "bmp581_types.hpp"
#include "bmp581_version.h"

#include <cstddef>
#include <cstdint>

class InstalledPort : public bmp581::BusInterface<InstalledPort> {
 public:
  bool WriteRegs(uint8_t reg, const uint8_t* data, std::size_t len) noexcept {
    (void)reg;
    (void)data;
    (void)len;
    return true;
  }
  bool ReadRegs(uint8_t reg, uint8_t* out, std::size_t len) noexcept {
    (void)reg;
    (void)out;
    (void)len;
    return true;
  }
  void DelayUs(uint32_t us) noexcept { (void)us; }
  bool EnsureInitialized() noexcept { return true; }
};

template class bmp581::Driver<InstalledPort>;
