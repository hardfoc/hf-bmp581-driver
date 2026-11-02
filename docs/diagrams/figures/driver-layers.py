"""The three layers of the driver: hardware-agnostic core, register transport, platform port.

Reads: From top to bottom. Your application talks to one bmp581::Driver<Bus> object, whose public calls are listed
from inc/bmp581_driver.hpp. Each call becomes register reads and writes in the order the datasheet requires (the
standby hop, IIR writes in standby, POR and NVM checks). The register transport (BusInterface in
inc/bmp581_bus_interface.hpp) is a CRTP base with four methods. The platform port is the only code you write: a class
that implements them over I2C or SPI. Below it is the sensor.
"""

import re
from pathlib import Path

from hardfoc_mkdocs_theme.figkit import Fig

ROOT = Path(__file__).resolve().parents[3]
DRIVER = "inc/bmp581_driver.hpp"
BUS = "inc/bmp581_bus_interface.hpp"


def data():
    drv = (ROOT / DRIVER).read_text()
    public = drv.split("public:", 1)[1].split("private:", 1)[0]
    calls = []
    for name in re.findall(r"^    (?:static )?[\w:<>]+ (\w+)\([^;]*?\)(?: const)? noexcept", public, re.M):
        if name != "Driver" and name not in calls:
            calls.append(name)
    bus = (ROOT / BUS).read_text()
    cls = bus.split("class BusInterface", 1)[1]
    hooks = re.findall(r"^    \w+ (\w+)\([^)]*\) noexcept \{", cls, re.M)
    return {
        "calls": calls,
        "port_hooks": hooks,
        "sources": [f"{DRIVER}:Driver public members", f"{BUS}:BusInterface"],
    }


def build(d):
    f = Fig(
        "driver-layers",
        "Three layers, one of them yours",
        alt="Layer diagram: the application calls the hardware-agnostic BMP581 driver, which reads and writes "
        "registers through the CRTP register transport, which calls the platform port you write, which drives the "
        "I2C or SPI bus of the BMP581.",
        width=1200,
        kicker="Architecture",
        subtitle="The driver and the transport are the same on every MCU. The port is a small class with four methods.",
        profile="overview",
        legend=[("brand", "this library"), ("teal", "you write"), ("slate", "hardware")],
    )
    W = f.body_w
    app = f.card(0, 0, W, "Your application", ["altitude, air-density compensation, weather station ..."],
                 tone="slate")
    top = app.b + 40
    chips_h = f.chips(16, f.region_inner_top(top), W - 32, [c + "()" for c in d["calls"]], tone="brand")
    reg = f.region(0, top, W, f.region_inner_top(top) - top + chips_h + 18, "Hardware-agnostic core", tone="brand",
                   sub="bmp581::Driver<Bus>, inc/bmp581_driver.hpp")
    bus = f.card(0, reg.b + 40, W, "Register transport",
                 ["BusInterface<Bus>: CRTP base, no virtual calls; register map and bit fields in",
                  "bmp581_registers.hpp, results and configuration in bmp581_types.hpp"],
                 tone="brand", accent=True)
    port = f.card(0, bus.b + 40, W, "Platform port (your class)", ["  ".join(h + "()" for h in d["port_hooks"])],
                  tone="teal", accent=True, mono=True)
    hw = f.card(0, port.b + 40, W, "BMP581", ["I2C up to 1 MHz at 0x46 / 0x47, or SPI mode 0 / 3 up to 12 MHz  /  INT"],
                tone="slate")
    for a, b in ((app, reg), (reg, bus), (bus, port), (port, hw)):
        f.connect(a, b, "bottom", "top", tone="rail")
    return f
