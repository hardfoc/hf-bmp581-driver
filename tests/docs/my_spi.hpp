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
    if (!platform_spi_transfer(tx, rx, 2 * len)) {
      return false;
    }
    if (reg == bmp581::reg::kCmd && data[0] == bmp581::cmd::kSoftReset) {
      // A soft reset returns the interface to I2C mode; one dummy read selects SPI again.
      platform_delay_us(bmp581::timing::kSoftResetUs);
      return DummyRead();
    }
    return true;
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

  // The chip powers up in I2C mode. The first SPI read switches it to SPI; its data is invalid.
  bool EnsureInitialized() noexcept {
    if (!spi_selected_) {
      spi_selected_ = DummyRead();
    }
    return spi_selected_;
  }

private:
  bool DummyRead() noexcept {
    uint8_t tx[2] = {static_cast<uint8_t>(bmp581::reg::kChipId | 0x80U), 0};
    uint8_t rx[2] = {};
    return platform_spi_transfer(tx, rx, sizeof(tx));
  }

  bool spi_selected_ = false;
};
// --8<-- [end:port]
