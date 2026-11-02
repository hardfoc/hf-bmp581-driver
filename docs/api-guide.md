# API guide

How to do the common jobs with the driver: initialise, configure, read, use the interrupt pin, read NVM and handle
errors.

Every snippet on this page is compiled by the host CI from
[`tests/docs/api_tasks.cpp`](../tests/docs/api_tasks.cpp), with `Driver` standing for `bmp581::Driver<MyI2c>`.
The [API reference](api-reference.md) lists every member.

## The shape of the API

One `bmp581::Driver<Bus>` object per sensor. It holds a reference to your port and the chip id; everything else
lives in the chip's registers.

| Group | Calls | Registers |
| --- | --- | --- |
| Lifecycle | `Init()`, `SoftReset()`, `Configure(Config)`, `ChipId()`, `Initialized()` | CHIP_ID, STATUS, CMD, INT_STATUS |
| Power mode | `GetPowerMode()`, `SetPowerMode(PowerMode)` | ODR_CONFIG |
| Oversampling and data rate | `GetOsrOdr()`, `SetOsrOdr(OsrOdrConfig)`, `GetEffectiveOsr()` | OSR_CONFIG, ODR_CONFIG, OSR_EFF |
| IIR filter | `GetIir()`, `SetIir(IirConfig)` | DSP_CONFIG, DSP_IIR |
| Interrupts | `ConfigureInterrupt(InterruptConfig)`, `SelectInterruptSources(InterruptSources)`, `ReadInterruptStatus()`, `DataReady()` | INT_CONFIG, INT_SOURCE, INT_STATUS |
| Data | `ReadSample(pressure_enabled)`, `DecodeSample(bytes, pressure_enabled)` | TEMP_DATA, PRESS_DATA |
| NVM | `ReadNvm(row)` | NVM_ADDR, NVM_DATA, CMD |
| Raw access | `ReadRegister(reg)`, `WriteRegister(reg, value)` | any |

Every call returns `bmp581::DriverResult<T>`: `.value` and `.error`, and it converts to `bool`. Check it: `false`
means a transfer failed or the chip did not confirm the step.

## Initialise

```cpp
--8<-- "tests/docs/api_tasks.cpp:init"
```

`Init()` is safe to call again: it clears `ChipId()` first and fills it only on success. `SoftReset()` returns every
register to its reset value, so call it before `Configure()`, never after. Over I2C the chip does not acknowledge the
reset command; the port must count that write as sent ([Porting](porting.md#i2c-port)).

## Configure

```cpp
--8<-- "tests/docs/api_tasks.cpp:configure"
```

`Configure()` writes standby, then oversampling and data rate, then the IIR filter, then the power mode, and stops
at the first failure. Pick a pair of oversampling ratios the data rate allows ([Register
interface](register-interface.md#oversampling-and-data-rate)), then check `odr_valid`: when it is `false` the chip
runs with default ratios, not yours. [Configuration](configuration.md) has presets for common uses.

## Read

```cpp
--8<-- "tests/docs/api_tasks.cpp:read"
```

`DataReady()` reads INT_STATUS, and reading INT_STATUS clears it. Do not read INT_STATUS anywhere else between
two samples, or you lose the flag. `ReadSample(false)` skips the pressure conversion when you disabled pressure
with `OsrOdrConfig::pressure_enable = false`.

## Take one sample on demand

```cpp
--8<-- "tests/docs/api_tasks.cpp:forced"
```

Forced mode does one conversion and returns to standby: the lowest average current for slow, irregular sampling.
The wait is t_startup (3 ms) plus the conversion time for the oversampling you set (datasheet Table 4).

## Use the interrupt pin

```cpp
--8<-- "tests/docs/api_tasks.cpp:interrupt"
```

In pulsed mode INT pulses for 90 µs to 121 µs per event; in latched mode it stays active until you read
`ReadInterruptStatus()`. Use open drain when several devices share the line, and pull it up to VDDIO.

## Read the NVM user words

```cpp
--8<-- "tests/docs/api_tasks.cpp:nvm"
```

The NVM holds three 16-bit user words at rows `0x20` to `0x22`. `ReadNvm()` switches to standby for the read and
restores the previous mode. Writing the NVM is not wrapped.

## Use any register directly

```cpp
--8<-- "tests/docs/api_tasks.cpp:raw"
```

`ReadRegister()` and `WriteRegister()` reach anything the driver does not wrap: FIFO, out-of-range detection, pad
drive strength. The addresses and masks are in [`bmp581_registers.hpp`](../inc/bmp581_registers.hpp). Write
standby-only registers in standby.

## Handle errors

```cpp
--8<-- "tests/docs/api_tasks.cpp:errors"
```

| `DriverError` | Returned by | Meaning |
| --- | --- | --- |
| `NotInitialized` | `Init()` | Your port's `EnsureInitialized()` returned `false` |
| `Bus` | any call | `ReadRegs()` or `WriteRegs()` returned `false` |
| `InvalidChipId` | `Init()` | CHIP_ID read 0, or not `0x50` / `0x51` |
| `NvmNotReady` | `Init()`, `ReadNvm()` | `nvm_rdy` clear or `nvm_err` set, after one soft-reset retry in `Init()` |
| `PorSoftReset` | `SoftReset()` | INT_STATUS `por` not set 2 ms after the reset command |
| `NvmInvalidAddr` | `ReadNvm()` | Row outside `0x20`..`0x22` |
| `NvmCommandError` | `Init()`, `ReadNvm()` | `nvm_cmd_err`: an NVM command outside standby |
| `InvalidPowerMode` | `SetPowerMode()` | A value outside the `PowerMode` enum |

`ToString(DriverError)` returns the enumerator's name for logs. [Troubleshooting](troubleshooting.md) maps
symptoms to causes.
