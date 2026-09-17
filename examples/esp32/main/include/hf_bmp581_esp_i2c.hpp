/**
 * @file hf_bmp581_esp_i2c.hpp
 * @brief Reusable ESP-IDF I2C register transport for `bmp581::Driver` (CRTP).
 */
#pragma once

#include "bmp581.hpp"

#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cstddef>
#include <cstdint>

namespace hf_bmp581_examples {

/**
 * @brief Template I2C bridge: one concrete type per (port, SDA, SCL, address, speed).
 *
 * Call `Install()` once before constructing `bmp581::Driver<Bmp581EspIdfI2c<...>>`.
 * Address 0x46 (SDO low) or 0x47 (SDO high). Up to 1 MHz Fm+.
 */
template <i2c_port_t kPort, int kSdaGpio, int kSclGpio, std::uint8_t kAddr7 = bmp581::addr::kSecondary,
          std::uint32_t kSpeedHz = 400000U, std::uint32_t kTimeoutMs = 20U>
class Bmp581EspIdfI2c
    : public bmp581::BusInterface<Bmp581EspIdfI2c<kPort, kSdaGpio, kSclGpio, kAddr7, kSpeedHz, kTimeoutMs>> {
public:
    static esp_err_t Install() noexcept {
        i2c_config_t cfg = {};
        cfg.mode = I2C_MODE_MASTER;
        cfg.sda_io_num = kSdaGpio;
        cfg.scl_io_num = kSclGpio;
        cfg.sda_pullup_en = GPIO_PULLUP_ENABLE;
        cfg.scl_pullup_en = GPIO_PULLUP_ENABLE;
        cfg.master.clk_speed = kSpeedHz;
        esp_err_t err = i2c_param_config(kPort, &cfg);
        if (err != ESP_OK) {
            return err;
        }
        err = i2c_driver_install(kPort, I2C_MODE_MASTER, 0, 0, 0);
        installed_ = (err == ESP_OK);
        return err;
    }

    bool WriteRegs(std::uint8_t reg, const std::uint8_t* data, std::size_t len) noexcept {
        if (len > 15) {
            return false;
        }
        std::uint8_t tx[16];
        tx[0] = reg;
        for (std::size_t i = 0; i < len; ++i) {
            tx[1 + i] = data[i];
        }
        return i2c_master_write_to_device(kPort, kAddr7, tx, len + 1, pdMS_TO_TICKS(kTimeoutMs)) == ESP_OK;
    }

    bool ReadRegs(std::uint8_t reg, std::uint8_t* out, std::size_t len) noexcept {
        return i2c_master_write_read_device(kPort, kAddr7, &reg, 1, out, len, pdMS_TO_TICKS(kTimeoutMs)) ==
               ESP_OK;
    }

    void DelayUs(std::uint32_t us) noexcept {
        if (us >= 1000U) {
            vTaskDelay(pdMS_TO_TICKS((us + 999U) / 1000U));
        } else {
            esp_rom_delay_us(us);
        }
    }

    bool EnsureInitialized() noexcept { return installed_; }

private:
    static inline bool installed_ = false;
};

}  // namespace hf_bmp581_examples
