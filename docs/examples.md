# Examples

Complete ESP32-S3 applications that run the driver on a BMP581 over I2C or SPI, one task each.

They run on an ESP32-S3 board wired to a BMP581 breakout. The I2C apps use I2C0 with SDA on GPIO 8 and SCL on
GPIO 9 at 400 kHz, address 0x47 (SDO high), through
[`hf_bmp581_esp_i2c.hpp`](../examples/esp32/main/include/hf_bmp581_esp_i2c.hpp). The SPI app uses SPI2 with CS on
GPIO 10, MOSI (SDI) on GPIO 11, SCLK on GPIO 12 and MISO (SDO) on GPIO 13, mode 0 at 10 MHz, through
[`hf_bmp581_esp_spi.hpp`](../examples/esp32/main/include/hf_bmp581_esp_spi.hpp). CI builds every app below on each
pull request.

## The apps

<!-- generated: examples -->
| App | What it shows | Source | CI |
| --- | --- | --- | --- |
| `bmp581_minimal_example` :material-star:{ title="featured" } | BMP581: init, soft reset, 100 Hz barometric preset, 1 Hz pressure/temperature — I2C0 SDA=GPIO8 SCL=GPIO9 @400k, addr 0x47 | [`bmp581_minimal_example.cpp`](../examples/esp32/main/bmp581_minimal_example.cpp) | yes |
| `bmp581_baro_demo` :material-star:{ title="featured" } | Data-ready polling at 100 Hz ODR, IIR bypass, per-second mean/RMS/span noise statistics | [`bmp581_baro_demo.cpp`](../examples/esp32/main/bmp581_baro_demo.cpp) | yes |
| `bmp581_spi_example` | BMP581 over SPI: init, verified soft reset with SPI re-selection, 100 Hz barometric preset, 1 Hz print — SPI2 CS=GPIO10 MOSI=GPIO11 SCLK=GPIO12 MISO=GPIO13, mode 0 @10 MHz | [`bmp581_spi_example.cpp`](../examples/esp32/main/bmp581_spi_example.cpp) | yes |

Build and flash one (from `examples/esp32`, ESP-IDF environment loaded):

```bash
./scripts/build_app.sh bmp581_minimal_example Release
./scripts/flash_app.sh flash_monitor bmp581_minimal_example Release
```

Source of this table: [`examples/esp32/app_config.yml`](../examples/esp32/app_config.yml).
<!-- /generated -->

## Set up once

```bash
git submodule update --init examples/esp32/scripts   # the HardFOC ESP-IDF build scripts
cd examples/esp32
. $IDF_PATH/export.sh                                 # ESP-IDF v5.5
./scripts/build_app.sh list                           # every app and build type
```

The scripts read [`app_config.yml`](../examples/esp32/app_config.yml) (the same file this page's table comes
from) and build each app as its own firmware image.

## What to read in an example

| You want to | Read |
| --- | --- |
| See the shortest complete bring-up: init, reset, preset, read | `bmp581_minimal_example.cpp` |
| Poll data-ready at 100 Hz and measure the sensor's own noise | `bmp581_baro_demo.cpp` |
| Run the driver over SPI, including the soft reset that returns the chip to I2C mode | `bmp581_spi_example.cpp` |
| Write an I2C port for ESP-IDF | `main/include/hf_bmp581_esp_i2c.hpp` |
| Write an SPI port for ESP-IDF (`spi_master`) | `main/include/hf_bmp581_esp_spi.hpp` |
| Move the pins, the address or the bus speed | The `using Bus = Bmp581EspIdfI2c<...>` (or `Bmp581EspIdfSpi<...>`) line in each app |

The [example folder's README](../examples/esp32/README.md) has the wiring table and the build commands.
