/**
 * @file bmp581.hpp
 * @brief Umbrella header for the HF-BMP581 barometric pressure sensor driver.
 *
 * @details Pulls in `bmp581::BusInterface`, the register map, typed
 *          configuration / sample structs, and `bmp581::Driver<BusT>`.
 *          See `examples/esp32/` for an ESP-IDF I2C adapter.
 *
 * @copyright Copyright (c) 2026 HardFOC. All rights reserved.
 */
#pragma once

#include "bmp581_bus_interface.hpp"
#include "bmp581_driver.hpp"
#include "bmp581_registers.hpp"
#include "bmp581_types.hpp"
#include "bmp581_version.h"

namespace bmp581 {

/** @brief Driver version string (from generated @ref bmp581_version.h). */
inline const char* GetDriverVersion() noexcept {
    return HF_BMP581_VERSION_STRING;
}

}  // namespace bmp581
