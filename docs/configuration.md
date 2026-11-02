# Configuration

The settings that decide noise, bandwidth, data rate and current: what each one does, the struct that holds it, and
presets for common uses.

Every snippet here is compiled from [`tests/docs/configuration.cpp`](../tests/docs/configuration.cpp).

## The preset

```cpp
--8<-- "tests/docs/configuration.cpp:preset"
```

`BarometricReferencePreset()` is for a pressure reference sampled by a control loop: pressure oversampling ×8
(0.30 Pa RMS typical), temperature ×1, `Odr::Hz100_2` (100.3 Hz; the pair allows up to 140 Hz), IIR coefficient 3
on both channels (-3 dB at about 4.6 Hz), normal mode.

## Choosing settings

| Goal | Pressure OSR / temperature OSR | ODR | IIR | Power mode |
| --- | --- | --- | --- | --- |
| Pressure reference for a control loop | ×8 / ×1 | `Hz100_2` | `Coeff3` | `Normal` |
| Lowest noise, slow (weather) | ×128 / ×8 | `Hz5` | `Coeff15` or more | `Normal` |
| Fast altitude changes (drones) | ×1 / ×1 | `Hz240` | `Bypass` or `Coeff1` | `Normal` |
| Lowest current, irregular samples | ×1 / ×1 | ignored | `Bypass` | `Forced` per sample |
| As fast as the oversampling allows | any | ignored | any | `Continuous` |

Each row stays within the datasheet's limit for its oversampling pair; the full table is on
[Register interface](register-interface.md#oversampling-and-data-rate). Ambient pressure noise (doors, wind) is
usually larger than the sensor's own: filter it with the IIR rather than with more oversampling.

## Building a configuration in pieces

```cpp
--8<-- "tests/docs/configuration.cpp:pieces"
```

`Configure(Config)` does the same in one call. Use the individual setters to change one thing at run time:
`SetOsrOdr()` keeps the power mode, `SetIir()` goes through standby and comes back.

## `Config`

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `osr_odr` | `OsrOdrConfig` | see below | Oversampling, pressure enable, data rate |
| `iir` | `IirConfig` | see below | IIR filter |
| `power_mode` | `PowerMode` | `Normal` | Written last |

## `OsrOdrConfig`

| Field | Type | Default | Register |
| --- | --- | --- | --- |
| `osr_temperature` | `Oversampling` | `X1` | `OSR_CONFIG.osr_t` |
| `osr_pressure` | `Oversampling` | `X1` | `OSR_CONFIG.osr_p` |
| `pressure_enable` | `bool` | `true` | `OSR_CONFIG.press_en`; `false` measures temperature only |
| `odr` | `Odr` | `Hz50` | `ODR_CONFIG.odr`; used in normal mode only |

## `IirConfig`

| Field | Type | Default | Register |
| --- | --- | --- | --- |
| `temperature` | `IirFilter` | `Bypass` | `DSP_IIR.set_iir_t` |
| `pressure` | `IirFilter` | `Bypass` | `DSP_IIR.set_iir_p` |
| `shadow_iir_temperature` | `bool` | `true` | `DSP_CONFIG.shdw_sel_iir_t`: data registers show the filtered temperature |
| `shadow_iir_pressure` | `bool` | `true` | `DSP_CONFIG.shdw_sel_iir_p`: data registers show the filtered pressure |
| `flush_forced_enable` | `bool` | `false` | `DSP_CONFIG.iir_flush_forced_en`: restart the filter on each forced conversion |

## `InterruptConfig` and `InterruptSources`

| Field | Default | Meaning |
| --- | --- | --- |
| `InterruptConfig::mode` | `Pulsed` | `Pulsed` (about 105 µs pulse) or `Latched` (until INT_STATUS is read) |
| `InterruptConfig::polarity` | `ActiveHigh` | `ActiveHigh` or `ActiveLow` |
| `InterruptConfig::drive` | `PushPull` | `PushPull` or `OpenDrain` |
| `InterruptConfig::enable` | `false` | INT pin enabled |
| `InterruptSources::data_ready` | `false` | New sample in the data registers |
| `InterruptSources::fifo_full`, `fifo_threshold` | `false` | FIFO events (the FIFO itself is not wrapped) |
| `InterruptSources::pressure_out_of_range` | `false` | Out-of-range detection (configure it with `WriteRegister()`) |

## Where the defaults come from

Each field's default is in its struct in [`inc/bmp581_types.hpp`](../inc/bmp581_types.hpp) and in the
[API reference](api-reference.md); the chip's own reset values are on [Register
interface](register-interface.md#register-map).
