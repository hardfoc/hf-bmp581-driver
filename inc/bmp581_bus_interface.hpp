/**
 * @file bmp581_bus_interface.hpp
 * @brief CRTP register transport for the Bosch BMP581 (I2C or SPI).
 *
 * @details The BMP581 is register addressed, so one adapter shape serves both
 *          buses. The platform adapter implements:
 *
 *            - `bool WriteRegs(uint8_t reg, const uint8_t* data, std::size_t len);`
 *            - `bool ReadRegs(uint8_t reg, uint8_t* out, std::size_t len);`
 *            - `void DelayUs(uint32_t us);`
 *            - `bool EnsureInitialized();`
 *
 *          I2C: `WriteRegs` sends `[reg, data…]`; `ReadRegs` writes `reg` then
 *          reads `len` bytes (7-bit address 0x46 or 0x47 selected by SDO).
 *          SPI (4-wire): the adapter must set bit 7 of `reg` for reads
 *          (`reg | 0x80`) and discard the first dummy byte the device clocks
 *          out after the register byte; writes use `reg & 0x7F`. Register
 *          auto-increment applies on both buses.
 *
 * @copyright Copyright (c) 2026 HardFOC. All rights reserved.
 */
#pragma once

#include <cstddef>
#include <cstdint>

namespace bmp581 {

/**
 * @brief CRTP base for the BMP581 register transport.
 * @tparam Derived Concrete adapter type (e.g. `EspI2cAdapter`).
 */
template <typename Derived>
class BusInterface {
public:
    /// Write @p len bytes starting at register @p reg (auto-increment).
    bool WriteRegs(uint8_t reg, const uint8_t* data, std::size_t len) noexcept {
        return static_cast<Derived*>(this)->WriteRegs(reg, data, len);
    }

    /// Read @p len bytes starting at register @p reg (auto-increment).
    bool ReadRegs(uint8_t reg, uint8_t* out, std::size_t len) noexcept {
        return static_cast<Derived*>(this)->ReadRegs(reg, out, len);
    }

    /// Blocking delay in microseconds (soft reset 2 ms, standby 2.5 ms, NVM 0.8 ms).
    void DelayUs(uint32_t us) noexcept { static_cast<Derived*>(this)->DelayUs(us); }

    /// Lazy bus bring-up; must return `true` before any transfer.
    bool EnsureInitialized() noexcept {
        return static_cast<Derived*>(this)->EnsureInitialized();
    }
};

}  // namespace bmp581
