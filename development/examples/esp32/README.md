# ESP32 examples — HF-BMP581 (Bosch barometric pressure sensor)

ESP-IDF examples for the **BMP581** over I2C. Default address **0x47** (SDO high);
use `0x46` with SDO low. **400 kHz** I2C (device supports 1 MHz Fm+).

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

| MCU signal | GPIO (default) | Sensor |
|------------|----------------|--------|
| SDA        | **8**          | SDA    |
| SCL        | **9**          | SCL    |
| 3V3        | VDD / VDDIO    | 1.71–3.6 V |
| GND        | GND            | GND    |

Breakouts (SparkFun SEN-20170, Adafruit 6407) include pull-ups and default to
0x47. Change the `using Bus = Bmp581EspIdfI2c<...>` alias to move pins, address
or speed.

## Apps (`app_config.yml`)

| App | Role |
|-----|------|
| `bmp581_minimal_example` | init, reset, barometric preset, 1 Hz print |
| `bmp581_baro_demo` | data-ready polling at 100 Hz, IIR bypass, noise statistics |

```bash
./scripts/build_app.sh list
./scripts/build_app.sh bmp581_minimal_example Debug
./scripts/flash_app.sh bmp581_minimal_example Debug
```
