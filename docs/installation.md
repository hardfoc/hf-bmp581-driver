---
layout: default
title: "Installation"
nav_order: 1
parent: "Documentation"
permalink: /docs/installation/
---

# Installation

The driver is **header-only**. CMake generates `bmp581_version.h` and exposes an
`INTERFACE` target `hf::bmp581` for consumers.

## Requirements

- C++17 compiler (GCC 9+, Clang 10+, ESP-IDF toolchains)
- CMake 3.16+
- An I2C or SPI master able to do register writes / auto-increment reads

## As a git submodule

```bash
git submodule add https://github.com/hardfoc/hf-bmp581-driver.git external/hf-bmp581-driver
git submodule update --init --recursive
```

```cmake
add_subdirectory(external/hf-bmp581-driver)
target_link_libraries(my_firmware PRIVATE hf::bmp581)
```

## Without `add_subdirectory`

```cmake
include(external/hf-bmp581-driver/cmake/hf_bmp581_build_settings.cmake)
target_include_directories(my_firmware PRIVATE ${HF_BMP581_PUBLIC_INCLUDE_DIRS})
```

## Host tests

```bash
cmake -S . -B build -DHF_BMP581_BUILD_TESTS=ON
cmake --build build && ctest --test-dir build
```

The tests run against a register-map fake bus: init / reset, standby hop,
OSR/ODR and IIR bit packing, interrupt configuration, 24-bit sample decode
(101325 Pa → `0x62F340`), NVM read.

## Repository layout

```
inc/          bmp581.hpp (umbrella), bmp581_driver.hpp, bmp581_types.hpp,
              bmp581_registers.hpp, bmp581_bus_interface.hpp, bmp581_version.h.in
cmake/        build settings + package config template
docs/         this documentation
examples/     ESP-IDF (ESP32-S3) examples + scripts submodule
tests/host/   hardware-free unit tests
_config/      Doxygen, Jekyll, lint configuration
```

**Next:** [Quick start →](quickstart.md)
