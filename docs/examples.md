---
layout: default
title: "Examples"
nav_order: 7
parent: "Documentation"
permalink: /docs/examples/
---

# Examples

## ESP32-S3 (`examples/esp32`)

| App | Role |
|-----|------|
| `bmp581_minimal_example` | init, reset, barometric preset, 1 Hz pressure/temperature |
| `bmp581_baro_demo` | data-ready polling at 100 Hz, IIR bypass, noise statistics |

```bash
cd examples/esp32
git submodule update --init --recursive
./scripts/build_app.sh bmp581_minimal_example Debug
./scripts/flash_app.sh bmp581_minimal_example Debug
```

Wiring and address details: [Hardware setup](hardware_setup.md) and
[`examples/esp32/README.md`](../examples/esp32/README.md).

## Transport adapter

`examples/esp32/main/include/hf_bmp581_esp_i2c.hpp` — `Bmp581EspIdfI2c<port, sda, scl, addr, hz>`
implements the four CRTP calls with the legacy ESP-IDF `driver/i2c.h` API.

**Next:** [Troubleshooting →](troubleshooting.md)
