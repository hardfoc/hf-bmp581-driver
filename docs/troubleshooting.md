# Troubleshooting

Symptoms you may see while bringing up a BMP581, their causes and the fix for each, cheapest check first.

Start with the `DriverError` of the failing call (`ToString(r.error)`): `Bus` and `NotInitialized` are transport
problems; the others mean the chip answered but did not confirm a step.

## `Init()` fails

| Error | Cause | Fix |
| --- | --- | --- |
| `NotInitialized` | Your port's `EnsureInitialized()` returned `false` | Install or open the bus before `Init()` |
| `Bus` | No acknowledge at the address, or a dead bus | Address from SDO (0x46 low, 0x47 high), pull-ups to VDDIO, CSB tied to VDDIO for I2C |
| `InvalidChipId` | Another part at that address, SDO floating, or the first SPI read after power-up | Check the address; on SPI do one dummy read before `Init()` ([Porting](porting.md#spi-port)) |
| `NvmNotReady` | The NVM had not loaded, or `nvm_err` is set | `Init()` already soft-resets once when STATUS reads 0. Wait at least 2 ms (t_powup) after power-up; if it persists, the part is faulty |
| `NvmCommandError` | `nvm_cmd_err` left over from an earlier NVM command | Soft reset, then `Init()` again |

## `SoftReset()` fails

| Error | Cause | Fix |
| --- | --- | --- |
| `Bus` over I2C | The chip does not acknowledge the reset command and your port reports it | Count the write of 0xB6 to CMD as sent ([Porting](porting.md#i2c-port)) |
| `PorSoftReset` over SPI | The reset returned the chip to I2C mode, so the INT_STATUS read was the invalid switching read | Do a dummy read after the reset in the port |
| `PorSoftReset` | Something else read INT_STATUS in between (reading clears it) | Do not poll INT_STATUS from another task during the reset |

## Readings

| Symptom | Cause | Fix |
| --- | --- | --- |
| The values never change | The chip is in standby or deep standby | Check the result of `Configure()` or `SetPowerMode(PowerMode::Normal)` |
| One new value, then nothing | Forced mode: one conversion, then standby | Set forced mode again per sample, or use normal mode |
| `DataReady()` never turns `true` | Another reader clears INT_STATUS first, or the chip is not converting | Read INT_STATUS in one place only; check the power mode |
| Pressure reads 0 | Pressure disabled (`pressure_enable = false`) or `ReadSample(false)` | Enable pressure in `OsrOdrConfig` |
| `odr_valid` is `false` | The oversampling pair does not fit the ODR period; the chip runs with ×1 / ×1 or ×2 / ×2 | Lower the oversampling or the ODR ([Register interface](register-interface.md#oversampling-and-data-rate)) |
| Noisy pressure | Low oversampling, or air movement | Raise pressure oversampling, enable the IIR filter (`SetIir()`), shield the port from drafts |
| Offset of tens of pascals | Solder drift (±30 Pa) or self-heating | Compare against a reference once after assembly; keep the sensor away from warm parts |

## SPI only

| Symptom | Cause | Fix |
| --- | --- | --- |
| Two-byte settings come back wrong | The port sends `[reg] [d0] [d1]`; SPI writes do not auto-increment | Send one address byte before each data byte |
| Every read is shifted by one byte | The port drops a "dummy" byte the BMP581 does not send | Take data from the byte right after the address |
| Works until the first soft reset | A soft reset selects I2C again | Dummy read after the reset |

## Next

[Hardware](hardware.md) has the pins and bus settings; [Porting](porting.md) the transport rules.
