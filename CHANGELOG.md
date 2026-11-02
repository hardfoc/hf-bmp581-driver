# Changelog

All notable changes to this project are documented in this file. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Changed

- Documentation moved to the HardFOC MkDocs platform: new page set (quickstart, hardware, architecture, register
  interface, API guide, configuration, porting, testing), code snippets compiled by the host build from
  `tests/docs/`, Doxygen under `/api/`, versioned with mike. The Jekyll site files and the vendored
  doxygen-awesome theme are gone.
- Host unit tests and the documentation snippets run in CI (`ci-host-tests.yml`).
- Documentation corrected against the BMP581 datasheet (BST-BMP581-DS004-13): SPI reads have no dummy byte, SPI
  writes do not auto-increment, a soft reset is not acknowledged over I2C and returns the interface to I2C mode,
  power-up time is 2 ms (3 ms is the start-up time from standby).

## [1.0.0]

### Added

- Header-only C++17 driver `bmp581::Driver<Bus>` over a CRTP register transport (I2C or SPI): chip-id and
  NVM-ready initialisation, soft reset with the POR check, power modes with the standby hop, OSR / ODR, IIR,
  interrupt pin and sources, data-ready polling, 24-bit pressure and temperature, NVM user-word read, raw register
  access.
- `Init()` soft-resets once and checks again when STATUS reads 0 with a valid chip id.
- `BarometricReferencePreset()` (100 Hz, pressure OSR ×8, IIR 3).
- Host unit tests against a register-map fake; ESP32-S3 examples `bmp581_minimal_example` and `bmp581_baro_demo`.
