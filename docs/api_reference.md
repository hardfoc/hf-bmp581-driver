---
layout: default
title: "API reference"
nav_order: 6
parent: "Documentation"
permalink: /docs/api-reference/
---

# API reference

Authoritative declarations live under [`inc/`](https://github.com/N3b3x/hf-bmp581-driver/tree/main/inc).
Run **Doxygen** (`doxygen _config/Doxyfile`) for HTML cross-links.

## Transport — `bmp581::BusInterface<Derived>`

CRTP base. Implement:

- `bool WriteRegs(uint8_t reg, const uint8_t* data, size_t len);`
- `bool ReadRegs(uint8_t reg, uint8_t* out, size_t len);`
- `void DelayUs(uint32_t us);`
- `bool EnsureInitialized();`

## Driver — `bmp581::Driver<BusT>`

| Method | Description |
|--------|-------------|
| `Init()` | chip id + NVM ready; `ChipId()` afterwards |
| `SoftReset()` | CMD 0xB6 with POR check |
| `Configure(Config)` | standby → OSR/ODR → IIR → power mode |
| `GetPowerMode()` / `SetPowerMode(mode)` | with mandatory standby hop |
| `GetOsrOdr()` / `SetOsrOdr(cfg)` | OSR_CONFIG / ODR_CONFIG |
| `GetEffectiveOsr()` | OSR_EFF incl. `odr_valid` |
| `GetIir()` / `SetIir(cfg)` | DSP_CONFIG / DSP_IIR (standby-only, mode restored) |
| `ConfigureInterrupt(cfg)` / `SelectInterruptSources(src)` | INT_CONFIG / INT_SOURCE |
| `ReadInterruptStatus()` / `DataReady()` | INT_STATUS |
| `ReadSample(pressure_enabled = true)` | 6-byte burst → `Sample` |
| `DecodeSample(bytes, pressure_enabled)` | static decode helper (tests) |
| `ReadNvm(addr)` | user word 0x20..0x22 |
| `ReadRegister(r)` / `WriteRegister(r, v)` | raw access |

## Types (`bmp581_types.hpp`)

- `DriverResult<T>` / `DriverError` — `None, InvalidParameter, NotInitialized, Bus, InvalidChipId, NvmNotReady, PorSoftReset, InvalidPowerMode, NvmInvalidAddr, NvmCommandError`
- `OsrOdrConfig`, `IirConfig`, `EffectiveOsr`, `InterruptConfig`, `InterruptSources`, `Sample`, `Config`
- `BarometricReferencePreset()` — 100 Hz, OSR ×8/×1, IIR 3, Normal

## Enumerations (`bmp581_registers.hpp`)

`PowerMode`, `Oversampling`, `Odr` (+ `OdrHz()`), `IirFilter`, `IntMode`,
`IntPolarity`, `IntDrive`; namespaces `addr`, `chip_id`, `reg`, `cmd`, `timing`,
`status`, `int_status`, `int_source`, bit-field masks.

**Next:** [Examples →](examples.md)
