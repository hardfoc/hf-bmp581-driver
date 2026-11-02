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
 *          I2C: `WriteRegs` sends `[reg, data…]` (auto-increment); `ReadRegs`
 *          writes `reg` then reads `len` bytes (7-bit address 0x46 or 0x47
 *          selected by SDO).
 *          SPI: reads send `reg | 0x80` and the data follows the address
 *          byte directly (no dummy byte), with auto-increment. Writes do not
 *          auto-increment: send `(reg + i) & 0x7F` before each data byte.
 *          The device starts in I2C mode after power-up and after a soft
 *          reset; one dummy SPI read selects SPI (its data is invalid).
 *          I2C: the device does not acknowledge a write of 0xB6 to CMD.
 *
 *          The adapter may also declare which bus it drives:
 *
 *            - `static constexpr bmp581::Interface kInterface = bmp581::Interface::Spi;`
 *
 *          The driver uses it for the two interface-specific steps: on SPI it
 *          performs the interface-selecting dummy read in `Init()` and after
 *          a soft reset; on I2C it accepts the missing acknowledge of the
 *          soft-reset command. An adapter without `kInterface` gets both
 *          (`Interface::Unspecified`), which is correct on either bus.
 *          Neither step is left to the adapter.
 *
 * @copyright Copyright (c) 2026 HardFOC. All rights reserved.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace bmp581 {

/// Bus an adapter drives, declared as `static constexpr Interface kInterface` (optional).
enum class Interface : uint8_t {
    Unspecified = 0,  ///< not declared: the driver handles both I2C and SPI behaviour
    I2c = 1,
    Spi = 2,
};

namespace detail {

template <typename Bus, typename = void>
struct DeclaredInterface {
    static constexpr Interface value = Interface::Unspecified;
};

template <typename Bus>
struct DeclaredInterface<Bus, std::void_t<decltype(Bus::kInterface)>> {
    static_assert(std::is_same_v<std::remove_cv_t<decltype(Bus::kInterface)>, Interface>,
                  "bmp581: the adapter's kInterface must be a bmp581::Interface");
    static constexpr Interface value = Bus::kInterface;
};

}  // namespace detail

/// The interface adapter @p Bus declares, or `Interface::Unspecified`.
template <typename Bus>
inline constexpr Interface kBusInterface = detail::DeclaredInterface<Bus>::value;

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
