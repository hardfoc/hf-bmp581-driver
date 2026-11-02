/**
 * @file hf_bmp581_esp_spi.hpp
 * @brief Reusable ESP-IDF SPI register transport for `bmp581::Driver` (CRTP, `spi_master`).
 *
 * @details BMP581 SPI protocol (datasheet BST-BMP581-DS004, section 5.5):
 *          - SPI mode 0 or mode 3 (the chip detects which from SCK at the CSB
 *            falling edge), MSB first, up to 12 MHz with VDDIO >= 1.62 V
 *            (Table 18).
 *          - Read: one address byte with bit 7 set (`reg | 0x80`), then the
 *            data, auto-incremented (Figures 10 and 11); no dummy byte.
 *          - Write: one `(reg & 0x7F, data)` pair per byte, all in one CSB-low
 *            transaction; writes do not auto-increment (Figure 9).
 *          - After writing DRIVE_CONFIG, INT_CONFIG or the NVM data registers,
 *            CSB stays high for at least 1 us before they are read back
 *            (section 5.5.4); the port waits 1 us after every write.
 *          The port declares `kInterface = Interface::Spi`, so the driver does
 *          the dummy read that selects SPI after power-up and after a soft
 *          reset (section 5.1).
 */
#pragma once

#include <cstddef>
#include <cstdint>

#include "bmp581.hpp"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace hf_bmp581_examples {

/**
 * @brief Template SPI bridge: one concrete type per (host, pins, clock, mode).
 *
 * Call `Install()` once before constructing `bmp581::Driver<Bmp581EspIdfSpi<...>>`.
 * It initialises the SPI host (no DMA: the longest transfer is 7 bytes) and
 * adds the BMP581 as a device with hardware chip select.
 */
template <spi_host_device_t kHost, int kMosiGpio, int kMisoGpio, int kSclkGpio, int kCsGpio,
          int kClockHz = 10 * 1000 * 1000, std::uint8_t kSpiMode = 0>
class Bmp581EspIdfSpi
    : public bmp581::BusInterface<
          Bmp581EspIdfSpi<kHost, kMosiGpio, kMisoGpio, kSclkGpio, kCsGpio, kClockHz, kSpiMode>> {
  static_assert(kSpiMode == 0 || kSpiMode == 3,
                "BMP581 supports SPI mode 0 or mode 3 only (section 5.5)");
  static_assert(kClockHz > 0 && kClockHz <= 12 * 1000 * 1000,
                "BMP581 SPI clock is at most 12 MHz (Table 18)");

public:
  /// SPI: the driver performs the interface-selecting dummy read in Init() and after SoftReset().
  static constexpr bmp581::Interface kInterface = bmp581::Interface::Spi;

  static constexpr std::size_t kMaxTransfer = 16;

  static esp_err_t Install() noexcept {
    spi_bus_config_t bus = {};
    bus.mosi_io_num = kMosiGpio;
    bus.miso_io_num = kMisoGpio;
    bus.sclk_io_num = kSclkGpio;
    bus.quadwp_io_num = -1;
    bus.quadhd_io_num = -1;
    bus.max_transfer_sz = static_cast<int>(kMaxTransfer);
    esp_err_t err = spi_bus_initialize(kHost, &bus, SPI_DMA_DISABLED);
    if (err != ESP_OK) {
      return err;
    }
    spi_device_interface_config_t dev = {};
    dev.mode = kSpiMode;
    dev.clock_speed_hz = kClockHz;
    dev.spics_io_num = kCsGpio;
    dev.queue_size = 1;
    err = spi_bus_add_device(kHost, &dev, &device_);
    if (err != ESP_OK) {
      (void)spi_bus_free(kHost);
      device_ = nullptr;
    }
    return err;
  }

  /// Burst write: `(reg + i) & 0x7F, data[i]` for each byte, in one transaction.
  bool WriteRegs(std::uint8_t reg, const std::uint8_t* data, std::size_t len) noexcept {
    if (device_ == nullptr || len == 0 || 2 * len > kMaxTransfer) {
      return false;
    }
    std::uint8_t tx[kMaxTransfer];
    for (std::size_t i = 0; i < len; ++i) {
      tx[2 * i] = static_cast<std::uint8_t>((reg + i) & 0x7FU);
      tx[2 * i + 1] = data[i];
    }
    const bool ok = Transfer(tx, nullptr, 2 * len);
    esp_rom_delay_us(1);  // CSB idle >= 1 us before a read-back (section 5.5.4)
    return ok;
  }

  /// Read: `reg | 0x80`, then @p len data bytes (auto-increment, no dummy byte).
  bool ReadRegs(std::uint8_t reg, std::uint8_t* out, std::size_t len) noexcept {
    if (device_ == nullptr || len == 0 || len + 1 > kMaxTransfer) {
      return false;
    }
    std::uint8_t tx[kMaxTransfer] = {};
    std::uint8_t rx[kMaxTransfer] = {};
    tx[0] = static_cast<std::uint8_t>(reg | 0x80U);
    if (!Transfer(tx, rx, len + 1)) {
      return false;
    }
    for (std::size_t i = 0; i < len; ++i) {
      out[i] = rx[i + 1];
    }
    return true;
  }

  void DelayUs(std::uint32_t us) noexcept {
    if (us >= 1000U) {
      vTaskDelay(pdMS_TO_TICKS((us + 999U) / 1000U));
    } else {
      esp_rom_delay_us(us);
    }
  }

  bool EnsureInitialized() noexcept { return device_ != nullptr; }

private:
  static bool Transfer(const std::uint8_t* tx, std::uint8_t* rx, std::size_t len) noexcept {
    spi_transaction_t t = {};
    t.length = len * 8U;
    t.tx_buffer = tx;
    t.rx_buffer = rx;
    return spi_device_polling_transmit(device_, &t) == ESP_OK;
  }

  static inline spi_device_handle_t device_ = nullptr;
};

}  // namespace hf_bmp581_examples
