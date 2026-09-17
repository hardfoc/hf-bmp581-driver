---
layout: default
title: "Quick start"
nav_order: 2
parent: "Documentation"
permalink: /docs/quickstart/
---

# Quick start

You supply a **CRTP** adapter that inherits `bmp581::BusInterface<YourType>` and
implements four calls: `WriteRegs`, `ReadRegs`, `DelayUs`, `EnsureInitialized`.

```cpp
#include "bmp581.hpp"

class MyBus : public bmp581::BusInterface<MyBus> {
public:
    bool WriteRegs(uint8_t reg, const uint8_t* data, size_t len) noexcept { /* [reg, data…] */ }
    bool ReadRegs(uint8_t reg, uint8_t* out, size_t len) noexcept        { /* write reg, read len */ }
    void DelayUs(uint32_t us) noexcept                                    { /* RTOS / busy delay */ }
    bool EnsureInitialized() noexcept                                     { return true; }
};
```

For SPI, set bit 7 of `reg` on reads (`reg | 0x80`) and drop the dummy byte the
device clocks out after the address; writes use `reg & 0x7F`.

## First sample

```cpp
MyBus bus;
bmp581::Driver<MyBus> dev(bus);

auto r = dev.Init();                              // chip id 0x50/0x51 + NVM ready
r = dev.SoftReset();                              // CMD 0xB6, 2 ms, POR bit checked
r = dev.Configure(bmp581::BarometricReferencePreset());   // 100 Hz, OSR x8, IIR 3, Normal

auto rdy = dev.DataReady();                       // INT_STATUS.drdy
if (rdy.ok() && rdy.value) {
    auto s = dev.ReadSample();                    // s.value.pressure_pa, temperature_c
}
```

## Choosing a configuration

| Goal | OSR p / t | ODR | IIR |
|------|-----------|-----|-----|
| Barometric reference for a control loop | ×8 / ×1 | 100 Hz | 3 |
| Lowest noise, slow | ×128 / ×8 | 5 Hz | 127 |
| Fast altitude / dynamics | ×1 / ×1 | 240 Hz | bypass |

After `SetOsrOdr()` check `GetEffectiveOsr().value.odr_valid`: the device
downgrades OSR when the combination cannot meet the ODR.

## Result handling

Every call returns `bmp581::DriverResult<T>`: check `.ok()` / `.error`.
`ToString(error)` gives a stable string for logs.

**Next:** [Register interface →](register_interface.md)
