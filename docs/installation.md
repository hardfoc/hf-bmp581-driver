# Installation

Add the driver to a CMake or ESP-IDF project. It is a header-only C++17 library with no dependencies.

## Requirements

- A C++17 compiler: GCC 9+, Clang 10+ or the ESP-IDF toolchains.
- CMake 3.16 or newer, if you use the CMake target.
- No RTOS, heap or exceptions: every call is `noexcept` and returns a `DriverResult`.

## Get the source

A git submodule keeps the driver pinned to a version you chose:

```bash
git submodule add https://github.com/hardfoc/hf-bmp581-driver.git external/hf-bmp581-driver
git submodule update --init external/hf-bmp581-driver
```

Only `inc/`, `cmake/` and `CMakeLists.txt` are needed to build. The examples have their own submodule
(`examples/esp32/scripts`); you do not need it to use the driver.

## CMake

```cmake
add_subdirectory(external/hf-bmp581-driver)
target_link_libraries(my_app PRIVATE hf::bmp581)
```

`hf::bmp581` (alias of `hf_bmp581`) is an `INTERFACE` target. It carries the include directories, including the
folder with the generated `bmp581_version.h`, and requires C++17 (`cxx_std_17`).

| Option | Default | Effect |
| --- | --- | --- |
| `HF_BMP581_ENABLE_WARNINGS` | `OFF` | `-Wall -Wextra -Wpedantic` on targets that link the driver |
| `HF_BMP581_BUILD_TESTS` | `OFF` | Builds the host unit tests and the documentation snippets ([Testing and CI](testing.md)) |

To use an installed copy instead, install it once and find the package:

```bash
cmake -S external/hf-bmp581-driver -B build-bmp581
cmake --install build-bmp581 --prefix /opt/hf
```

```cmake
find_package(hf_bmp581 CONFIG REQUIRED)        # with /opt/hf on CMAKE_PREFIX_PATH
target_link_libraries(my_app PRIVATE hf::bmp581)
```

`cmake --install` copies the headers of `inc/` and the generated `bmp581_version.h` into `include/`, and the package
files into `lib/cmake/hf_bmp581/`. The installed target has the same name, `hf::bmp581`.

Without `add_subdirectory`, include the settings file and use its variables:

```cmake
include(external/hf-bmp581-driver/cmake/hf_bmp581_build_settings.cmake)
target_include_directories(my_app PRIVATE ${HF_BMP581_PUBLIC_INCLUDE_DIRS})
target_compile_features(my_app PRIVATE cxx_std_17)
```

## ESP-IDF

The ESP-IDF component wrapper is [`examples/esp32/components/hf_bmp581/`](../examples/esp32/components/hf_bmp581/CMakeLists.txt).
Copy that folder into your project's `components/` (or point `EXTRA_COMPONENT_DIRS` at it) and set the
`HF_BMP581_ROOT` cache variable to the driver checkout; the wrapper stops with a clear message when the path is
wrong. Your component then declares `REQUIRES hf_bmp581`. The wrapper requires the IDF `driver` component, which
the example I2C port uses.

## How the build is organised

| Layer | File | Role |
| --- | --- | --- |
| Settings | [`cmake/hf_bmp581_build_settings.cmake`](../cmake/hf_bmp581_build_settings.cmake) | Target name, include directories, version: the single source of truth |
| Library | [`CMakeLists.txt`](../CMakeLists.txt) | `INTERFACE` target, install rules and package files |
| ESP-IDF | [`examples/esp32/components/hf_bmp581/CMakeLists.txt`](../examples/esp32/components/hf_bmp581/CMakeLists.txt) | IDF component wrapper over the same settings |

| Variable | Value |
| --- | --- |
| `HF_BMP581_TARGET_NAME` | `hf_bmp581` |
| `HF_BMP581_VERSION` | Semantic version, written into `bmp581_version.h` |
| `HF_BMP581_PUBLIC_INCLUDE_DIRS` | `inc/` and the generated-header folder |
| `HF_BMP581_SOURCE_FILES` | Empty: the driver is header-only |
| `HF_BMP581_IDF_REQUIRES` | `driver` |

The version is set once in the settings file and written into `bmp581_version.h` at configure time. Read it at
run time:

```cpp
const char* v = bmp581::GetDriverVersion();   // "1.0.0"
```

## Repository layout

| Path | Contents |
| --- | --- |
| `inc/` | `bmp581.hpp` (umbrella), driver, types, register map, transport, version template |
| `cmake/` | Build settings and the package configuration template |
| `docs/` | This site |
| `examples/esp32/` | ESP-IDF applications and the build-scripts submodule |
| `tests/host/`, `tests/docs/` | Host unit tests and the compiled documentation snippets |
| `_config/` | Lint configurations |

## Next

[Quickstart](quickstart.md) wires the sensor and reads it; [Porting](porting.md) writes the register transport for
your MCU.
