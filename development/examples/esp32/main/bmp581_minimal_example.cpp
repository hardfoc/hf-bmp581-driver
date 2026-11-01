// =============================================================================
// HF-BMP581 — minimal ESP-IDF example
// =============================================================================
// Default wiring (ESP32-S3): I2C0, SDA=GPIO8, SCL=GPIO9, 400 kHz, address 0x47.
// Init (chip id + NVM ready), soft reset, barometric preset (100 Hz, OSR x8,
// IIR 3), then prints pressure / temperature at 1 Hz.
// =============================================================================

#include "bmp581.hpp"
#include "hf_bmp581_esp_i2c.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

constexpr const char* TAG = "Bmp581Min";
using Bus = hf_bmp581_examples::Bmp581EspIdfI2c<I2C_NUM_0, 8, 9>;

}  // namespace

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "HF-BMP581 minimal (I2C0 SDA=8 SCL=9 @400k, addr 0x%02X, driver %s)",
             bmp581::addr::kSecondary, bmp581::GetDriverVersion());

    ESP_ERROR_CHECK(Bus::Install());
    Bus bus;
    bmp581::Driver<Bus> dev(bus);

    auto r = dev.Init();
    if (!r.ok()) {
        ESP_LOGE(TAG, "init failed: %s", bmp581::ToString(r.error).data());
        return;
    }
    ESP_LOGI(TAG, "chip id 0x%02X", dev.ChipId());

    r = dev.SoftReset();
    if (!r.ok()) {
        ESP_LOGW(TAG, "soft reset: %s", bmp581::ToString(r.error).data());
    }
    r = dev.Configure(bmp581::BarometricReferencePreset());
    if (!r.ok()) {
        ESP_LOGE(TAG, "configure failed: %s", bmp581::ToString(r.error).data());
        return;
    }
    const auto eff = dev.GetEffectiveOsr();
    if (eff.ok()) {
        ESP_LOGI(TAG, "ODR valid=%d eff OSR p=%d t=%d", eff.value.odr_valid,
                 static_cast<int>(eff.value.osr_pressure), static_cast<int>(eff.value.osr_temperature));
    }

    while (true) {
        const auto s = dev.ReadSample();
        if (!s.ok()) {
            ESP_LOGW(TAG, "read failed: %s", bmp581::ToString(s.error).data());
        } else {
            ESP_LOGI(TAG, "P=%.2f Pa (%.3f hPa)  T=%.2f C", s.value.pressure_pa, s.value.pressure_pa / 100.0f,
                     s.value.temperature_c);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
