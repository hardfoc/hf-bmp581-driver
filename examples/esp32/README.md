# ESP32 examples — HF-BMP581 (Bosch barometric pressure sensor)

ESP-IDF examples for the **BMP581** over I2C and SPI. I2C: default address **0x47** (SDO high);
use `0x46` with SDO low. **400 kHz** I2C (device supports 1 MHz Fm+). SPI: mode 0 at **10 MHz**
(device supports mode 0 or 3, up to 12 MHz).

## Prerequisites

- ESP-IDF **v5.4+** (CI uses `release/v5.5`)
- Python 3 with PyYAML (for `scripts/generate_matrix.py`)

## One-time setup

From `examples/esp32/`:

```bash
git submodule update --init --recursive
```

This pulls `scripts/` (**hf-espidf-project-tools**): `build_app.sh`, `flash_app.sh`,
and `generate_matrix.py` (same layout as other `hf-*-driver` submodules).

## Default wiring (ESP32-S3)

| ESP32-S3 | BMP581 | Notes |
| --- | --- | --- |
| GPIO 8 | SDI (SDA) | Pull-up to 3.3 V |
| GPIO 9 | SCK (SCL) | Pull-up to 3.3 V |
| 3.3 V | VDD, VDDIO, CSB, SDO | CSB high keeps I2C; SDO high selects address 0x47 |
| GND | VSS | |

Breakout boards such as the SparkFun SEN-20170 carry the pull-ups and default to 0x47. Change the
`using Bus = Bmp581EspIdfI2c<...>` alias to move pins, address or speed.

## SPI wiring (ESP32-S3, `bmp581_spi_example`)

| ESP32-S3 | BMP581 | Notes |
| --- | --- | --- |
| GPIO 10 | CSB | Chip select, active low |
| GPIO 11 | SDI | MOSI |
| GPIO 12 | SCK | Serial clock |
| GPIO 13 | SDO | MISO |
| 3.3 V | VDD, VDDIO | |
| GND | VSS | |

Change the `using Bus = Bmp581EspIdfSpi<host, mosi, miso, sclk, cs, clock_hz, mode>` alias to move pins or speed.

## Apps (`app_config.yml`)

| App | Role |
| --- | --- |
| `bmp581_minimal_example` | init, reset, barometric preset, 1 Hz print |
| `bmp581_baro_demo` | data-ready polling at 100 Hz, IIR bypass, noise statistics |
| `bmp581_spi_example` | the minimal flow over SPI (`spi_master`), soft reset with SPI re-selection |

```bash
./scripts/build_app.sh list
./scripts/build_app.sh bmp581_minimal_example Debug
./scripts/flash_app.sh flash_monitor bmp581_minimal_example Debug
```
