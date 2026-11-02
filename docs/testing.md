# Testing and CI

How the driver is checked: host unit tests against a model of the chip's registers, compiled documentation, ESP32
builds and static analysis, on every pull request.

## Host unit tests

The tests build the real driver against a fake bus that models the register map: writes land in a register array,
reads come from it, and writes to CMD act out a soft reset or an NVM read. It also models the bus around a reset:
over I2C the reset command is not acknowledged, over SPI the chip returns to I2C mode until a dummy read, and the
chip does not answer until 2 ms after the command. Delays are counted, not slept, so the tests check that the
driver waits the datasheet's times.

```bash
cmake -S . -B build-host -DHF_BMP581_BUILD_TESTS=ON
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

| Test | Covers |
| --- | --- |
| `TestInitAndReset` | Chip id, NVM-ready check, the cold-NVM soft-reset retry, soft reset with the POR check |
| `TestSoftResetI2cNack` | `SoftReset()` over I2C, where the fake does not acknowledge the reset command: accepted, 2 ms wait, CHIP_ID / STATUS / INT_STATUS read back |
| `TestSoftResetVerifies` | `InvalidChipId`, `NvmNotReady` and `PorSoftReset` when CHIP_ID, STATUS or INT_STATUS do not confirm the reset |
| `TestSoftResetBusErrors` | A failed read after the reset, a dead bus, and a failed reset write on SPI are reported as `Bus` |
| `TestSoftResetSpiReentry` | SPI wiring: the fake returns to I2C mode after the reset and answers the first read with invalid data; the driver's dummy read selects SPI again |
| `TestInitColdNvmOverI2c` | `Init()`'s recovery reset runs even though the reset command is not acknowledged |
| `TestPowerModes` | The standby hop and its delay, deep standby |
| `TestOsrOdrIir` | Bit packing of OSR_CONFIG, ODR_CONFIG, DSP_CONFIG and DSP_IIR; mode restore after `SetIir()` |
| `TestInterruptsAndSample` | INT_CONFIG and INT_SOURCE packing, data-ready, 24-bit decoding (101325 Pa as `0x62F340`) |
| `TestIntConfigFields` | INT_CONFIG field masks tile the register (`pad_int_drv` is bits 7:4); `ConfigureInterrupt()` keeps the pad drive bits |
| `TestNvm` | The NVM read sequence and address checks |

All tests are in [`tests/host/bmp581_driver_test.cpp`](../tests/host/bmp581_driver_test.cpp). A second ctest,
`hf_bmp581_install_consumer` ([`tests/install_consumer.cmake`](../tests/install_consumer.cmake)), runs
`cmake --install` into the build tree and builds a `find_package(hf_bmp581)` consumer that links `hf::bmp581`
and includes every public header from the installed copy.

## Documentation snippets

Every C++ block on this site is included from [`tests/docs/`](../tests/docs/my_i2c.hpp), which the host build
compiles (target `hf_bmp581_doc_snippets`, `-Wall -Wextra -Wpedantic`) with the driver instantiated over both
documented ports. A change to the API that breaks a page breaks the build first.

## CI workflows

| Workflow | What it checks |
| --- | --- |
| Host unit tests | GCC and Clang build and run the tests and the doc snippets |
| ESP32 examples build | Every app in `examples/esp32/app_config.yml`, for each listed ESP-IDF version and build type |
| C++ lint, C++ analysis | clang-format, clang-tidy and cppcheck |
| Docs | Strict site build, figure and generated-table drift, Doxygen, link check, Lighthouse budget |
| Markdown lint, YAML lint, link check | Repository text files |

The workflows call the shared, versioned workflows of
[hf-general-ci-tools](https://github.com/hardfoc/hf-general-ci-tools) and
[hf-espidf-ci-tools](https://github.com/hardfoc/hf-espidf-ci-tools).

## On hardware

The [ESP32 examples](examples.md) are the hardware check: `bmp581_minimal_example` prints pressure and temperature
once a second, `bmp581_baro_demo` prints the noise statistics at 100 Hz. Compare the RMS with the datasheet's
figure for the oversampling you set.
