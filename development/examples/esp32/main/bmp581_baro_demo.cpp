// =============================================================================
// HF-BMP581 — barometer demo: data-ready polling at ODR, noise statistics.
// =============================================================================
// I2C0 SDA=GPIO8 SCL=GPIO9 @400 kHz, address 0x47. Normal mode 100 Hz, OSR x8,
// IIR bypass so the printed RMS reflects the sensor, not the filter. Prints
// mean / RMS / min / max pressure each second, and the number of samples that
// were actually fresh (drdy) versus polled.
// =============================================================================

#include "bmp581.hpp"
#include "hf_bmp581_esp_i2c.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cfloat>
#include <cmath>

namespace {

constexpr const char* TAG = "Bmp581Demo";
using Bus = hf_bmp581_examples::Bmp581EspIdfI2c<I2C_NUM_0, 8, 9>;

}  // namespace

extern "C" void app_main(void) {
    ESP_ERROR_CHECK(Bus::Install());
    Bus bus;
    bmp581::Driver<Bus> dev(bus);
    if (!dev.Init().ok() || !dev.SoftReset().ok()) {
        ESP_LOGE(TAG, "init/reset failed");
        return;
    }

    bmp581::Config cfg = bmp581::BarometricReferencePreset();
    cfg.iir.pressure = bmp581::IirFilter::Bypass;
    cfg.iir.temperature = bmp581::IirFilter::Bypass;
    if (!dev.Configure(cfg).ok()) {
        ESP_LOGE(TAG, "configure failed");
        return;
    }
    bmp581::InterruptSources src{};
    src.data_ready = true;
    (void)dev.SelectInterruptSources(src);

    double sum = 0.0, sumsq = 0.0;
    float mn = FLT_MAX, mx = -FLT_MAX;
    int n = 0, polls = 0;
    TickType_t last = xTaskGetTickCount();

    while (true) {
        ++polls;
        const auto rdy = dev.DataReady();
        if (rdy.ok() && rdy.value) {
            const auto s = dev.ReadSample();
            if (s.ok()) {
                const float p = s.value.pressure_pa;
                sum += p; sumsq += static_cast<double>(p) * p; ++n;
                mn = p < mn ? p : mn; mx = p > mx ? p : mx;
            }
        }
        if (xTaskGetTickCount() - last >= pdMS_TO_TICKS(1000)) {
            if (n > 1) {
                const double mean = sum / n;
                const double rms = std::sqrt(sumsq / n - mean * mean);
                ESP_LOGI(TAG, "n=%d/%d mean=%.2f Pa rms=%.3f Pa span=%.2f Pa", n, polls, mean, rms, mx - mn);
            }
            sum = sumsq = 0.0; mn = FLT_MAX; mx = -FLT_MAX; n = polls = 0;
            last = xTaskGetTickCount();
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}
