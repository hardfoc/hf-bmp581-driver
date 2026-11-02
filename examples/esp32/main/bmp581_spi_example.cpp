// =============================================================================
// HF-BMP581 — SPI example (ESP-IDF spi_master)
// =============================================================================
// Default wiring (ESP32-S3): SPI2 (FSPI), CS=GPIO10, MOSI=GPIO11 (BMP581 SDI),
// SCLK=GPIO12 (SCK), MISO=GPIO13 (SDO), mode 0 at 10 MHz.
// Init (SPI select dummy read, chip id, NVM ready), soft reset (back to I2C
// mode, the driver's dummy read selects SPI again, reset verified), barometric
// preset (100 Hz, OSR x8, IIR 3), then prints pressure / temperature at 1 Hz.
// =============================================================================

#include "bmp581.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hf_bmp581_esp_spi.hpp"

namespace {

constexpr const char* TAG = "Bmp581Spi";
using Bus = hf_bmp581_examples::Bmp581EspIdfSpi<SPI2_HOST, 11, 13, 12, 10>;

}  // namespace

extern "C" void app_main(void) {
  ESP_LOGI(TAG, "HF-BMP581 SPI (SPI2 CS=10 MOSI=11 SCLK=12 MISO=13, mode 0 @10 MHz, driver %s)",
           bmp581::GetDriverVersion());

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
    ESP_LOGE(TAG, "soft reset failed: %s", bmp581::ToString(r.error).data());
    return;
  }
  ESP_LOGI(TAG, "soft reset verified (CHIP_ID, STATUS, INT_STATUS.por), SPI selected again");

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
      ESP_LOGI(TAG, "P=%.2f Pa (%.3f hPa)  T=%.2f C", s.value.pressure_pa,
               s.value.pressure_pa / 100.0f, s.value.temperature_c);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
