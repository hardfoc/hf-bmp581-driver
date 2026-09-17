---
layout: default
title: "CMake integration"
nav_order: 5
parent: "Documentation"
permalink: /docs/cmake-integration/
---

# CMake integration

## Standalone consumer

```cmake
add_subdirectory(path/to/hf-bmp581-driver)
target_link_libraries(app PRIVATE hf::bmp581)
```

## Settings file only

`cmake/hf_bmp581_build_settings.cmake` is `include_guard`ed and sets:

| Variable | Purpose |
|----------|---------|
| `HF_BMP581_TARGET_NAME` | `hf_bmp581` |
| `HF_BMP581_VERSION*` | semantic version → generated `bmp581_version.h` |
| `HF_BMP581_PUBLIC_INCLUDE_DIRS` | `inc/` + generated header dir |
| `HF_BMP581_SOURCE_FILES` | empty (header-only) |
| `HF_BMP581_IDF_REQUIRES` | `driver` (ESP-IDF component wrapper) |

## Options

| Option | Default | Effect |
|--------|---------|--------|
| `HF_BMP581_ENABLE_WARNINGS` | OFF | `-Wall -Wextra -Wpedantic` on consumers |
| `HF_BMP581_BUILD_TESTS` | OFF | builds `tests/host` and registers with CTest |

## ESP-IDF

`examples/esp32/components/hf_bmp581/CMakeLists.txt` wraps the settings file as
an IDF component; add the component directory to `EXTRA_COMPONENT_DIRS`.

**Next:** [API reference →](api_reference.md)
