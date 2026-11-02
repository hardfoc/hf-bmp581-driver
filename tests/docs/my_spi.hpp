/**
 * @file my_spi.hpp
 * @brief The SPI port shown on the documentation's Porting page, compiled by the host build.
 */
#pragma once

#include "bmp581.hpp"

#include <cstddef>
#include <cstdint>

// Platform hooks: your SDK's calls. Declared only, so the docs build checks the port compiles.
bool platform_spi_transfer(const uint8_t* tx, uint8_t* rx, std::size_t len) noexcept;  // CS low .. CS high
void platform_delay_us(uint32_t us) noexcept;

// --8<-- [start:port]
class MySpi : public bmp581::BusInterface<MySpi> {
public:
  // Tells the driver to select SPI with a dummy read after power-up and after a soft reset.
  static constexpr bmp581::Interface kInterface = bmp581::Interface::Spi;

  // SPI writes do not auto-increment: send one (address, data) pair per byte, bit 7 clear.
  bool WriteRegs(uint8_t reg, const uint8_t* data, std::size_t len) noexcept {
    uint8_t tx[16];
    uint8_t rx[16];
    if (2 * len > sizeof(tx)) {
      return false;
    }
    for (std::size_t i = 0; i < len; ++i) {
      tx[2 * i] = static_cast<uint8_t>((reg + i) & 0x7FU);
      tx[2 * i + 1] = data[i];
    }
    return platform_spi_transfer(tx, rx, 2 * len);
  }

  // Reads: address with bit 7 set, then the data bytes (auto-increment); no dummy byte.
  bool ReadRegs(uint8_t reg, uint8_t* out, std::size_t len) noexcept {
    uint8_t tx[16] = {};
    uint8_t rx[16] = {};
    if (len + 1 > sizeof(tx)) {
      return false;
    }
    tx[0] = static_cast<uint8_t>(reg | 0x80U);
    if (!platform_spi_transfer(tx, rx, len + 1)) {
      return false;
    }
    for (std::size_t i = 0; i < len; ++i) {
      out[i] = rx[i + 1];
    }
    return true;
  }

  void DelayUs(uint32_t us) noexcept { platform_delay_us(us); }

  // Called before every Init(); bring the bus up here if your SDK needs it.
  bool EnsureInitialized() noexcept { return true; }
};
// --8<-- [end:port]
