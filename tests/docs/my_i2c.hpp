/**
 * @file my_i2c.hpp
 * @brief The I2C port shown on the documentation's Porting page, compiled by the host build.
 *
 * Every platform call goes through the `platform_*` functions declared below; a real port replaces them with its
 * SDK (ESP-IDF, STM32 HAL, Zephyr, Linux i2c-dev). The class is what the driver needs, nothing more.
 */
#pragma once

#include "bmp581.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

// Platform hooks: your SDK's calls. Declared only, so the docs build checks the port compiles.
bool platform_i2c_write(uint8_t addr7, const uint8_t* data, std::size_t len) noexcept;  // START .. STOP
bool platform_i2c_write_read(uint8_t addr7, const uint8_t* tx, std::size_t tx_len, uint8_t* rx,
                             std::size_t rx_len) noexcept;  // write, repeated START, read
void platform_delay_us(uint32_t us) noexcept;

// --8<-- [start:port]
class MyI2c : public bmp581::BusInterface<MyI2c> {
public:
  // 7-bit address: 0x46 with SDO low, 0x47 with SDO high.
  explicit MyI2c(uint8_t address = bmp581::addr::kSecondary) noexcept : address_(address) {}

  // One transaction: [reg, data...]. The chip increments the register address after each byte.
  bool WriteRegs(uint8_t reg, const uint8_t* data, std::size_t len) noexcept {
    uint8_t frame[16];
    if (len + 1 > sizeof(frame)) {
      return false;
    }
    frame[0] = reg;
    std::memcpy(frame + 1, data, len);
    const bool acked = platform_i2c_write(address_, frame, len + 1);
    // The BMP581 does not acknowledge a soft-reset command over I2C: count that write as sent.
    const bool soft_reset = reg == bmp581::reg::kCmd && len == 1 && data[0] == bmp581::cmd::kSoftReset;
    return acked || soft_reset;
  }

  // Write the register address, repeated START, read len bytes (auto-increment).
  bool ReadRegs(uint8_t reg, uint8_t* out, std::size_t len) noexcept {
    return platform_i2c_write_read(address_, &reg, 1, out, len);
  }

  void DelayUs(uint32_t us) noexcept { platform_delay_us(us); }

  // Called before every Init(); bring the bus up here if your SDK needs it.
  bool EnsureInitialized() noexcept { return true; }

private:
  uint8_t address_;
};
// --8<-- [end:port]
