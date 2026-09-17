---
layout: default
title: "Troubleshooting"
nav_order: 8
parent: "Documentation"
permalink: /docs/troubleshooting/
---

# Troubleshooting

## `InvalidChipId`

- Wrong address (0x46 vs 0x47 — SDO level) or a different part on the bus.
- SPI: the first read after power-up can be invalid; issue a dummy read or call `Init()` twice.

## `NvmNotReady` on `Init()`

Power-up not complete (~3 ms) or a genuine NVM error. Retry after a delay; if
`nvm_err` persists the part is faulty.

## `PorSoftReset`

INT_STATUS did not report `por_softreset_complete` 2 ms after CMD 0xB6. Check
that the bus is not stuck and that nothing else read INT_STATUS in between
(reading clears it in pulsed mode).

## `odr_valid == false`

The OSR pair cannot be completed within the ODR period; the device lowered the
effective OSR. Reduce OSR or ODR (see the data-sheet OSR/ODR table).

## Readings do not change

- Power mode still Standby — call `SetPowerMode(Normal)` or `Configure()`.
- Forced mode gives one sample then returns to standby by design.

## Noisy pressure

Raise pressure OSR, enable IIR (`SetIir`), or average on the host. Remember
IIR is standby-only to write; the driver handles the hop.

**Back:** [Documentation home →](index.md)
