# Quickstart

Read pressure and temperature from your MCU: wire four signals, write a small port class, call four driver
functions.

## 1. Wire it

The quickstart uses I2C, the interface the sensor selects after power-up. [Hardware](hardware.md) has the SPI
wiring and the full pin list.

| MCU | BMP581 | Notes |
| --- | --- | --- |
| 3.3 V (or 1.8 V) | VDD (pin 10), VDDIO (pin 1) | 100 nF to ground on each |
| GND | VSS (pins 3, 8, 9) | |
| I2C SCL | SCK (pin 2) | Pull-up to VDDIO; up to 1 MHz, start at 400 kHz |
| I2C SDA | SDI (pin 4) | Pull-up to VDDIO |
| VDDIO or GND | SDO (pin 5) | High: address 0x47; low: 0x46. Do not leave it floating |
| VDDIO | CSB (pin 6) | Keeps the sensor in I2C mode |

Breakout boards such as the SparkFun SEN-20170 (default address 0x47) and the Adafruit 6407 carry the
pull-ups; check the board's jumpers for the address.

## 2. Add the driver

Add the repository as a submodule and link the `hf::bmp581` CMake target (or add `inc/` to your include path and
build with C++17). [Installation](installation.md) covers CMake and ESP-IDF.

```bash
git submodule add https://github.com/hardfoc/hf-bmp581-driver.git external/hf-bmp581-driver
```

## 3. Write the port

The port is the only platform code. It writes and reads registers and waits. Replace the three `platform_*` calls
with your SDK's.

```cpp title="my_i2c.hpp"
--8<-- "tests/docs/my_i2c.hpp:port"
```

[Porting](porting.md) explains each method and shows the SPI variant.

## 4. Read the sensor

```cpp title="main.cpp"
--8<-- "tests/docs/quickstart.cpp:quickstart"
```

What each step does:

1. `Init()` reads CHIP_ID (0x50) and checks that the NVM has loaded (`STATUS.nvm_rdy`, no `nvm_err`). Nothing else
   is worth calling until it succeeds.
2. `Configure()` puts the chip in standby, writes oversampling and output data rate, then the IIR filter, then
   switches to normal mode. The preset samples at 100 Hz with pressure oversampling ×8.
3. `DataReady()` reads INT_STATUS until the first conversion is done, about 10 ms after the mode change.
4. `ReadSample()` reads the six data registers in one burst and converts them: pressure / 64 in pascals,
   temperature / 65536 in degrees Celsius.

This example is built by the host CI, so it compiles against the current headers.

## If it does not work

| Symptom | First thing to check |
| --- | --- |
| `Init()` returns `Bus` | Address (0x46 or 0x47 from SDO), pull-ups, CSB tied to VDDIO |
| `Init()` returns `InvalidChipId` | Another part answers at that address, or SDO floats |
| `Init()` returns `NvmNotReady` | Supply ramp or a part still booting: wait 2 ms after power-up and retry |
| Pressure stays at the same value | The chip is still in standby: check the result of `Configure()` |

## Next

- [API guide](api-guide.md): forced mode, the interrupt pin, NVM, errors.
- [Configuration](configuration.md): choose oversampling, data rate and filter for your use.
- [Examples](examples.md): ESP32-S3 applications.
