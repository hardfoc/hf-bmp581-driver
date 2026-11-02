---
hide:
  - navigation
  - toc
---

<div class="hf-hero" markdown>
<span class="hf-kicker">HardFOC driver</span>

# BMP581 Driver

<p>A header-only C++17 driver for the Bosch BMP581 barometric pressure sensor. It checks the chip, sets oversampling,
output data rate, IIR filter and power mode in the order the datasheet requires, and reads pressure in pascals and
temperature in degrees Celsius, on any MCU with I2C or SPI.</p>

[Quickstart](quickstart.md){ .md-button .md-button--primary }
[API guide](api-guide.md){ .md-button }
[Source on GitHub](https://github.com/hardfoc/hf-bmp581-driver){ .md-button }
</div>

The driver has no platform code. You give it a small class that reads and writes registers over I2C or SPI and
waits; it does the rest: the chip-id and NVM checks, the standby hop before every mode change, IIR writes in
standby, 24-bit decoding and error codes.

<div class="hf-chip" markdown>
<div class="hf-chip__head">
<span class="hf-chip__kicker">Barometric pressure sensor</span>
<span class="hf-chip__name">BMP581</span>
<span class="hf-chip__vendor">Bosch Sensortec</span>
</div>

Measures
:   Absolute pressure 30 to 125 kPa and temperature -40 to 85 °C, 24-bit each

Accuracy
:   ±30 Pa absolute (typical, 300 to 1100 hPa, -5 to 65 °C); 0.08 Pa RMS noise at the highest resolution

Host interface
:   I2C up to 1 MHz at 0x46 or 0x47, SPI mode 0 or 3 up to 12 MHz (4- or 3-wire), I3C

Supply
:   VDD 1.71 V to 3.6 V, VDDIO 1.08 V to 3.6 V, independent

Output data rate
:   0.125 Hz to 240 Hz in normal mode, up to 480 Hz in continuous mode

Package
:   10-pin metal-lid LGA, 2.0 × 2.0 × 0.75 mm

Driver uses
:   Register access over I2C or SPI; data-ready polling or the INT pin. FIFO and out-of-range detection are not
    wrapped

[Datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp581-ds004.pdf) ·
[Product page](https://www.bosch-sensortec.com/products/environmental-sensors/pressure-sensors/bmp581/) ·
[Bosch BMP5 Sensor API](https://github.com/boschsensortec/BMP5_SensorAPI)
</div>

## What you get

- **One object per sensor.** `bmp581::Driver<YourBus>` with `Init()`, `Configure()`, `DataReady()` and
  `ReadSample()` for the common path. See [API guide](api-guide.md).
- **The datasheet's sequences, done for you.** Power-mode changes go through standby, IIR writes happen in standby
  and the previous mode comes back, NVM reads wait for `nvm_rdy`. See [Register interface](register-interface.md).
- **A preset for a pressure reference.** `BarometricReferencePreset()`: 100 Hz, pressure OSR ×8, IIR 3. See
  [Configuration](configuration.md).
- **Typed results.** Every call returns `DriverResult<T>` with a stable `DriverError` code and `ToString()` for logs.
- **Zero-overhead portability.** The port is a CRTP class with four methods: no virtual calls, no heap, no RTOS
  dependency. See [Porting](porting.md).
- **Tested without hardware.** Host unit tests run the driver against a register-map model of the chip. See
  [Testing and CI](testing.md).

## Where to go next

<div class="grid cards" markdown>

-   :material-rocket-launch-outline: **[Quickstart](quickstart.md)**

    ---

    Wire the sensor, write the port, read pressure in about 15 lines.

-   :material-chip: **[Hardware](hardware.md)**

    ---

    Pins, address selection, bus settings, timing and supply.

-   :material-layers-outline: **[Architecture](architecture.md)**

    ---

    The three layers and what each one owns.

-   :material-code-braces: **[API guide](api-guide.md)**

    ---

    Initialise, configure, read and handle errors, task by task.

-   :material-swap-horizontal: **[Porting](porting.md)**

    ---

    Implement the register transport for your MCU.

-   :material-book-open-variant: **[API reference](api-reference.md)**

    ---

    Every class and member, generated from the headers.

</div>
