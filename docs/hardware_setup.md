---
layout: default
title: "Hardware setup"
nav_order: 4
parent: "Documentation"
permalink: /docs/hardware-setup/
---

# Hardware setup

## Electrical

- VDD **1.71–3.6 V**, VDDIO **1.08–3.6 V** (independent).
- I2C up to **1 MHz** (Fm+); SPI 4-wire / 3-wire up to **12 MHz**; I3C.
- 2.0 × 2.0 × 0.75 mm metal-lid LGA — use a breakout for bench work.

## Address / interface select

| Pin | I2C | SPI |
|-----|-----|-----|
| CSB | high | chip select (active low) |
| SDO | low → **0x46**, high → **0x47** | MISO |

Breakouts: SparkFun SEN-20170 (Qwiic, default 0x47, jumper for 0x46 and SPI),
Adafruit 6407 (STEMMA QT, I2C or SPI).

## Placement for a barometric reference

- Vent the sensor to the volume whose pressure you want (enclosure ambient for
  an atmospheric reference); keep it away from fans and heat sources.
- Temperature-induced offset is ±0.5 Pa/K — thermal stability matters more
  than sampling rate.
- Solder drift is a few tenths of hPa; a one-point comparison against a
  reference after assembly is good practice when absolute accuracy matters.

## Performance (data sheet)

| Parameter | Value |
|-----------|-------|
| Range | 30–125 kPa (full accuracy 300–1100 hPa) |
| Absolute accuracy | ±30 Pa typ. (300–1100 hPa, −5…65 °C) |
| Relative accuracy | ±6 Pa per 10 kPa step |
| Noise | 0.08 Pa RMS (lowest bandwidth, highest resolution) |
| ODR | 0.125–240 Hz normal, up to 480 Hz continuous |

**Next:** [CMake integration →](cmake_integration.md)
