---
layout: default
title: "Register interface"
nav_order: 3
parent: "Documentation"
permalink: /docs/register-interface/
---

# Register interface

Source: Bosch BMP581 data sheet (BST-BMP581-DS004) and BMP5 Sensor API v1.1.1.
All values in `inc/bmp581_registers.hpp`.

## Register map (used subset)

| Reg | Name | Purpose |
|-----|------|---------|
| 0x01 | CHIP_ID | 0x50 / 0x51 |
| 0x02 | REV_ID | silicon revision |
| 0x14 | INT_CONFIG | mode / polarity / drive / enable |
| 0x15 | INT_SOURCE | drdy, fifo_full, fifo_thr, oor_p |
| 0x1D–0x1F | TEMP_DATA | 24-bit two's complement, 1/65536 °C |
| 0x20–0x22 | PRESS_DATA | 24-bit unsigned, 1/64 Pa |
| 0x27 | INT_STATUS | drdy, fifo, oor, **por_softreset_complete** (0x10) |
| 0x28 | STATUS | core_rdy, **nvm_rdy** (0x02), nvm_err (0x04), nvm_cmd_err (0x08) |
| 0x2B–0x2D | NVM_ADDR / NVM_DATA | user words 0x20..0x22 |
| 0x30 | DSP_CONFIG | IIR shadow / flush selects |
| 0x31 | DSP_IIR | IIR coefficients (t bits 2:0, p bits 5:3) |
| 0x36 | OSR_CONFIG | osr_t 2:0, osr_p 5:3, press_en 6 |
| 0x37 | ODR_CONFIG | pwr_mode 1:0, odr 6:2, deep_dis 7 |
| 0x38 | OSR_EFF | effective OSR + odr_is_valid (0x80) |
| 0x7E | CMD | 0xB6 soft reset; 0x5D / 0xA5 NVM read enable |

## Procedures (as ported)

- **Init** — read CHIP_ID (non-zero → check STATUS nvm_rdy && !nvm_err) → accept 0x50/0x51.
- **Soft reset** — CMD 0xB6, wait 2 ms, require INT_STATUS.por_softreset_complete.
- **Power mode** — read ODR_CONFIG; if not standby, write standby (deep_dis = 1) and wait
  t_standby 2.5 ms; then write the target mode. Deep standby = standby with deep_dis = 0
  (only reported when ODR ≤ 5 Hz, FIFO off, IIR bypass).
- **OSR / ODR** — ODR faster than 5 Hz requires deep standby disabled; the driver hops to
  plain standby first. Two-byte write of OSR_CONFIG + ODR_CONFIG preserving mode bits.
- **IIR** — writable in standby only; previous Normal/Forced/Continuous mode restored.
- **Interrupts** — INT_SOURCE = 0, read INT_STATUS to clear, then write INT_CONFIG.
- **NVM read** — standby → NVM_ADDR → CMD 0x5D, 0xA5 → wait 0.8 ms → nvm_rdy → NVM_DATA.

## Scaling

`pressure_pa = raw_u24 / 64`, `temperature_c = raw_s24 / 65536`.

## Timing

| Event | Time |
|-------|------|
| Soft reset | 2 ms |
| Standby settle | 2.5 ms |
| NVM read ready | 0.8 ms |
| Start-up (POR) | ~3 ms |

**Next:** [Hardware setup →](hardware_setup.md)
