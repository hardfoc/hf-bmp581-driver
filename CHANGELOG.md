# Changelog

All notable changes to this project are documented in this file. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added

- Optional `static constexpr bmp581::Interface kInterface` in the port (`Interface::I2c` or `Interface::Spi`) tells
  the driver which bus it drives; `bmp581::kBusInterface<Bus>` reads it. Without it the driver accepts the
  soft-reset command's missing ACK and does the SPI dummy read, which is correct on either bus. The documented
  I2C and SPI ports and the ESP32 I2C port declare it and no longer special-case the reset command.
- ESP32 SPI port `hf_bmp581_examples::Bmp581EspIdfSpi` on the ESP-IDF `spi_master` driver (read bit 0x80, one
  address byte per written byte, mode 0 or 3, at most 12 MHz, `kInterface = Interface::Spi`) and the
  `bmp581_spi_example` app, built in CI.

### Changed

- **Breaking (behaviour):** `SoftReset()` can now also fail with `InvalidChipId`, `NvmNotReady` or `NvmCommandError`
  (the post-reset checks of datasheet section 4.3.9), and a port that declares `Interface::Spi` gets `Bus` when the
  reset command itself fails to send. A port without `kInterface` sees one extra CHIP_ID read in `Init()` and in
  `SoftReset()` (the SPI dummy read).
- Documentation moved to the HardFOC MkDocs platform: new page set (quickstart, hardware, architecture, register
  interface, API guide, configuration, porting, testing), code snippets compiled by the host build from
  `tests/docs/`, Doxygen under `/api/`, versioned with mike. The Jekyll site files and the vendored
  doxygen-awesome theme are gone.
- Host unit tests and the documentation snippets run in CI (`ci-host-tests.yml`).
- Documentation corrected against the BMP581 datasheet (BST-BMP581-DS004-13): SPI reads have no dummy byte, SPI
  writes do not auto-increment, a soft reset is not acknowledged over I2C and returns the interface to I2C mode,
  power-up time is 2 ms (3 ms is the start-up time from standby).

### Fixed

- `cmake --install` installs the driver. It installed nothing: the package config was generated but not installed,
  no headers were copied and no target was exported, so `find_package(hf_bmp581)` could not work. It now installs
  every header the public headers include (`inc/*.hpp` and the generated `bmp581_version.h`) and exports the
  documented target `hf::bmp581`. A host test installs the build tree and builds a `find_package()` consumer.
- `SoftReset()` works over I2C. The chip does not acknowledge the write of 0xB6 to CMD (datasheet section 7.36), so
  every I2C port that reported the missing ACK made `SoftReset()` return `Bus`, and `Init()` skipped its recovery
  reset for a part whose NVM boot had not finished. The driver now accepts the failed status of that one write
  (unless the port declares SPI, which has no ACK), waits t_soft_res (2 ms, Table 4) and verifies the reset as
  section 4.3.9 recommends: CHIP_ID not 0, STATUS `nvm_rdy` = 1 and `nvm_err` = 0, INT_STATUS `por` = 1. Every
  other failed transfer, including those reads, is still reported as `Bus`.
- `SoftReset()` and `Init()` select SPI again. Power-up and a soft reset return the interface to I2C until a dummy
  SPI read (section 5.1, Table 16); the driver now does that read itself in `Init()` and after the reset, so the
  CHIP_ID / STATUS / INT_STATUS checks no longer read the invalid switching data over SPI.
- `int_config::kPadDriveMask` is `0xF0`, the `pad_int_drv` field at INT_CONFIG bits 7:4 (datasheet section 7.5).
  It was `0x70` and missed bit 7, the drive-strength MSB. Code that masks or sets the pad drive strength with it
  now reaches the whole field. New: `int_config::kPadDriveShift` (4).

## [1.0.0]

### Added

- Header-only C++17 driver `bmp581::Driver<Bus>` over a CRTP register transport (I2C or SPI): chip-id and
  NVM-ready initialisation, soft reset with the POR check, power modes with the standby hop, OSR / ODR, IIR,
  interrupt pin and sources, data-ready polling, 24-bit pressure and temperature, NVM user-word read, raw register
  access.
- `Init()` soft-resets once and checks again when STATUS reads 0 with a valid chip id.
- `BarometricReferencePreset()` (100 Hz, pressure OSR ×8, IIR 3).
- Host unit tests against a register-map fake; ESP32-S3 examples `bmp581_minimal_example` and `bmp581_baro_demo`.

<!-- markdownlint-configure-file {"MD024": {"siblings_only": true}} -->
