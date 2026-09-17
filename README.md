---
layout: default
title: "HardFOC BMP581 Driver"
description: "Hardware-agnostic C++17 driver for the Bosch BMP581 barometric pressure sensor (I2C / SPI)"
nav_order: 1
permalink: /
---

# HF-BMP581 Driver

**Header-only C++17 driver for the Bosch BMP581 barometric pressure sensor**:
chip-id / NVM-ready init, POR-checked soft reset, power modes with the mandatory
standby hop, OSR / ODR, IIR (standby-only write with mode restore), interrupt pin
and source configuration, data-ready polling, 24-bit pressure (1/64 Pa) and
temperature (1/65536 °C), NVM user-word read. Register sequences ported from the
Bosch BMP5 Sensor API (BSD-3).

[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![CI](https://github.com/N3b3x/hf-bmp581-driver/actions/workflows/esp32-examples-build-ci.yml/badge.svg?branch=main)](https://github.com/N3b3x/hf-bmp581-driver/actions/workflows/esp32-examples-build-ci.yml)
[![Docs](https://img.shields.io/badge/docs-GitHub%20Pages-blue)](https://n3b3x.github.io/hf-bmp581-driver/)

## Table of contents

1. [Overview](#overview)
2. [Features](#features)
3. [Quick start](#quick-start)
4. [Documentation](#documentation)
5. [Examples](#examples)
6. [Official references](#official-references)
7. [License](#license)

## Overview

> **[Live documentation (GitHub Pages)](https://n3b3x.github.io/hf-bmp581-driver/)** —
> Installation, register interface, CMake, API summary, and troubleshooting.

The driver targets an **absolute-pressure reference** in host firmware (density
compensation, gauge → absolute conversion, altitude). The BMP581 offers ±30 Pa
typical absolute accuracy, 0.08 Pa RMS noise and up to 240 Hz ODR; a
`BarometricReferencePreset()` (100 Hz, OSR ×8, IIR 3) is provided. One CRTP
register transport shape serves both I2C and SPI. FIFO and out-of-range
features are intentionally not ported.

## Features

- **CRTP** `bmp581::BusInterface<Derived>` — no virtual calls; you provide
  `WriteRegs` / `ReadRegs` / `DelayUs` / `EnsureInitialized`.
- **`bmp581::Driver<BusT>`** — `Init`, `SoftReset`, `Configure`, `Set/GetPowerMode`,
  `Set/GetOsrOdr`, `GetEffectiveOsr`, `Set/GetIir`, `ConfigureInterrupt`,
  `SelectInterruptSources`, `DataReady`, `ReadSample`, `ReadNvm`, raw register access.
- **Typed results** — `DriverResult<T>` with stable `DriverError` codes.
- **Complete register map** with bit-field masks and `OdrHz()` helper.
- **No heap allocation**; fits FreeRTOS / bare metal.
- **Host tests** (`-DHF_BMP581_BUILD_TESTS=ON`) against a register-map fake;
  **ESP32-S3** examples.

## Quick start

```cmake
add_subdirectory(/path/to/hf-bmp581-driver)
target_link_libraries(your_target PRIVATE hf::bmp581)
```

```cpp
#include "bmp581.hpp"

MyBus bus;                                   // inherits bmp581::BusInterface<MyBus>
bmp581::Driver<MyBus> dev(bus);
dev.Init();
dev.SoftReset();
dev.Configure(bmp581::BarometricReferencePreset());
auto s = dev.ReadSample();                   // s.value.pressure_pa, s.value.temperature_c
```

## Documentation

- [Installation](docs/installation.md) · [Quick start](docs/quickstart.md) ·
  [Register interface](docs/register_interface.md) · [Hardware setup](docs/hardware_setup.md) ·
  [CMake integration](docs/cmake_integration.md) · [API reference](docs/api_reference.md) ·
  [Examples](docs/examples.md) · [Troubleshooting](docs/troubleshooting.md) ·
  [Datasheets](docs/datasheet/README.md)

## Examples

`examples/esp32/` — `bmp581_minimal_example` (init + 1 Hz print) and
`bmp581_baro_demo` (data-ready polling, noise statistics). Build with
`./scripts/build_app.sh <app> Debug` after `git submodule update --init --recursive`.

## Official references

- Bosch BMP581 data sheet (BST-BMP581-DS004)
- [`boschsensortec/BMP5_SensorAPI`](https://github.com/boschsensortec/BMP5_SensorAPI) v1.1.1 (BSD-3) — see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)

## License

MIT — see [LICENSE](LICENSE). Register procedures ported from Bosch's BSD-3
Sensor API; notice reproduced in `THIRD_PARTY_NOTICES.md`.
