---
layout: default
title: "Documentation"
description: "Complete documentation for the HardFOC Bosch BMP581 barometric pressure sensor driver"
nav_order: 2
parent: "HardFOC BMP581 Driver"
permalink: /docs/
has_children: true
---

# HF-BMP581 driver documentation

This site mirrors the [`docs/`](https://github.com/N3b3x/hf-bmp581-driver/tree/main/docs) folder in the repository and documents the **Bosch BMP581** barometric pressure sensor register interface as implemented by this header-only driver (register sequences ported from the Bosch BMP5 Sensor API v1.1.1).

> **Browse on GitHub:** [repository home](https://github.com/N3b3x/hf-bmp581-driver) · [Issues](https://github.com/N3b3x/hf-bmp581-driver/issues)

## Documentation structure

### Getting started

1. **[Installation](installation.md)** — Toolchain, CMake, submodule layout
2. **[Quick start](quickstart.md)** — Minimal CRTP register adapter and first sample
3. **[Register interface](register_interface.md)** — Register map, power modes, OSR/ODR, IIR, interrupts, scaling

### Hardware and integration

4. **[Hardware setup](hardware_setup.md)** — Address select, I2C vs SPI, supply, mounting
5. **[CMake integration](cmake_integration.md)** — `hf_bmp581_build_settings.cmake`, `hf::bmp581`

### Reference and examples

6. **[API reference](api_reference.md)** — `Driver<BusT>`, types, results
7. **[Examples](examples.md)** — ESP32-S3 `build_app.sh` workflow
8. **[Troubleshooting](troubleshooting.md)** — Chip id, NVM, POR, ODR validity

### Manufacturer

9. **[Datasheet and links](datasheet/README.md)** — Bundled PDF and Bosch references

---

**Back:** [Repository README →](../README.md)
