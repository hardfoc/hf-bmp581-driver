/**
 * @file bmp581_registers.hpp
 * @brief BMP581 register map, bit fields, enumerations and timing constants.
 *
 * @details Values follow the Bosch BMP581 data sheet (BST-BMP581-DS004) and the
 *          Bosch BMP5 Sensor API (`bmp5_defs.h`, v1.1.1). Register access is
 *          identical over I2C and SPI (SPI reads set bit 7 of the address).
 *
 * @copyright Copyright (c) 2026 HardFOC. All rights reserved.
 */
#pragma once

#include <cstdint>

namespace bmp581 {

/// 7-bit I2C addresses (SDO low → 0x46, SDO high → 0x47).
namespace addr {
constexpr uint8_t kPrimary = 0x46;
constexpr uint8_t kSecondary = 0x47;
}  // namespace addr

/// Chip identifiers accepted by `Driver::Init()`.
namespace chip_id {
constexpr uint8_t kPrimary = 0x50;
constexpr uint8_t kSecondary = 0x51;
}  // namespace chip_id

/// Register addresses.
namespace reg {
constexpr uint8_t kChipId = 0x01;
constexpr uint8_t kRevId = 0x02;
constexpr uint8_t kChipStatus = 0x11;
constexpr uint8_t kDriveConfig = 0x13;
constexpr uint8_t kIntConfig = 0x14;
constexpr uint8_t kIntSource = 0x15;
constexpr uint8_t kFifoConfig = 0x16;
constexpr uint8_t kFifoCount = 0x17;
constexpr uint8_t kFifoSel = 0x18;
constexpr uint8_t kTempDataXlsb = 0x1D;  ///< 0x1D..0x1F temperature, 0x20..0x22 pressure
constexpr uint8_t kTempDataLsb = 0x1E;
constexpr uint8_t kTempDataMsb = 0x1F;
constexpr uint8_t kPressDataXlsb = 0x20;
constexpr uint8_t kPressDataLsb = 0x21;
constexpr uint8_t kPressDataMsb = 0x22;
constexpr uint8_t kIntStatus = 0x27;
constexpr uint8_t kStatus = 0x28;
constexpr uint8_t kFifoData = 0x29;
constexpr uint8_t kNvmAddr = 0x2B;
constexpr uint8_t kNvmDataLsb = 0x2C;
constexpr uint8_t kNvmDataMsb = 0x2D;
constexpr uint8_t kDspConfig = 0x30;
constexpr uint8_t kDspIir = 0x31;
constexpr uint8_t kOorThrPLsb = 0x32;
constexpr uint8_t kOorThrPMsb = 0x33;
constexpr uint8_t kOorRange = 0x34;
constexpr uint8_t kOorConfig = 0x35;
constexpr uint8_t kOsrConfig = 0x36;
constexpr uint8_t kOdrConfig = 0x37;
constexpr uint8_t kOsrEff = 0x38;
constexpr uint8_t kCmd = 0x7E;
}  // namespace reg

/// Command register values.
namespace cmd {
constexpr uint8_t kSoftReset = 0xB6;
constexpr uint8_t kNvmFirst = 0x5D;
constexpr uint8_t kNvmReadEnable = 0xA5;
constexpr uint8_t kNvmWriteEnable = 0xA0;
}  // namespace cmd

/// Timing (data sheet / Bosch API).
namespace timing {
constexpr uint32_t kSoftResetUs = 2000;
constexpr uint32_t kStandbyUs = 2500;
constexpr uint32_t kNvmReadyReadUs = 800;
constexpr uint32_t kNvmReadyWriteUs = 10000;
constexpr uint32_t kStartupUs = 3000;  ///< POR → ready
}  // namespace timing

/// NVM user space (3 × 16-bit words).
namespace nvm {
constexpr uint8_t kStartAddr = 0x20;
constexpr uint8_t kEndAddr = 0x22;
}  // namespace nvm

/// STATUS (0x28) bits.
namespace status {
constexpr uint8_t kCoreRdy = 0x01;
constexpr uint8_t kNvmRdy = 0x02;
constexpr uint8_t kNvmErr = 0x04;
constexpr uint8_t kNvmCmdErr = 0x08;
}  // namespace status

/// INT_STATUS (0x27) bits.
namespace int_status {
constexpr uint8_t kDataReady = 0x01;
constexpr uint8_t kFifoFull = 0x02;
constexpr uint8_t kFifoThreshold = 0x04;
constexpr uint8_t kPressureOor = 0x08;
constexpr uint8_t kPorSoftResetComplete = 0x10;
}  // namespace int_status

/// INT_SOURCE (0x15) enable bits.
namespace int_source {
constexpr uint8_t kDataReady = 0x01;
constexpr uint8_t kFifoFull = 0x02;
constexpr uint8_t kFifoThreshold = 0x04;
constexpr uint8_t kPressureOor = 0x08;
}  // namespace int_source

/// INT_CONFIG (0x14) fields.
namespace int_config {
constexpr uint8_t kModeMask = 0x01;      ///< 0 pulsed, 1 latched
constexpr uint8_t kPolMask = 0x02;       ///< 0 active-low, 1 active-high
constexpr uint8_t kOdMask = 0x04;        ///< 0 push-pull, 1 open-drain
constexpr uint8_t kEnableMask = 0x08;
constexpr uint8_t kPadDriveMask = 0x70;
}  // namespace int_config

/// OSR_CONFIG (0x36) fields.
namespace osr_config {
constexpr uint8_t kTempOsMask = 0x07;
constexpr uint8_t kPressOsMask = 0x38;
constexpr uint8_t kPressOsShift = 3;
constexpr uint8_t kPressEnMask = 0x40;
}  // namespace osr_config

/// ODR_CONFIG (0x37) fields.
namespace odr_config {
constexpr uint8_t kPowerModeMask = 0x03;
constexpr uint8_t kOdrMask = 0x7C;
constexpr uint8_t kOdrShift = 2;
constexpr uint8_t kDeepDisableMask = 0x80;  ///< 1 = deep standby disabled
}  // namespace odr_config

/// OSR_EFF (0x38) fields.
namespace osr_eff {
constexpr uint8_t kTempOsMask = 0x07;
constexpr uint8_t kPressOsMask = 0x38;
constexpr uint8_t kPressOsShift = 3;
constexpr uint8_t kOdrValidMask = 0x80;
}  // namespace osr_eff

/// DSP_CONFIG (0x30) fields.
namespace dsp_config {
constexpr uint8_t kIirFlushForcedEnMask = 0x04;
constexpr uint8_t kShadowIirTempMask = 0x08;
constexpr uint8_t kFifoIirTempMask = 0x10;
constexpr uint8_t kShadowIirPressMask = 0x20;
constexpr uint8_t kFifoIirPressMask = 0x40;
constexpr uint8_t kOorSelIirPressMask = 0x80;
}  // namespace dsp_config

/// DSP_IIR (0x31) fields.
namespace dsp_iir {
constexpr uint8_t kTempMask = 0x07;
constexpr uint8_t kPressMask = 0x38;
constexpr uint8_t kPressShift = 3;
}  // namespace dsp_iir

/// Power modes (ODR_CONFIG bits 1:0; deep standby = standby with deep_dis = 0).
enum class PowerMode : uint8_t {
    Standby = 0,
    Normal = 1,      ///< periodic at ODR
    Forced = 2,      ///< one conversion then standby
    Continuous = 3,  ///< back-to-back conversions
    DeepStandby = 4, ///< pseudo-mode: standby + deep_dis cleared
};

/// Oversampling (OSR_CONFIG).
enum class Oversampling : uint8_t {
    X1 = 0, X2 = 1, X4 = 2, X8 = 3, X16 = 4, X32 = 5, X64 = 6, X128 = 7,
};

/// Output data rate (ODR_CONFIG bits 6:2). Codes ascend as rate descends.
enum class Odr : uint8_t {
    Hz240 = 0x00, Hz218_5 = 0x01, Hz199_1 = 0x02, Hz179_2 = 0x03, Hz160 = 0x04,
    Hz149_3 = 0x05, Hz140 = 0x06, Hz129_8 = 0x07, Hz120 = 0x08, Hz110_1 = 0x09,
    Hz100_2 = 0x0A, Hz89_6 = 0x0B, Hz80 = 0x0C, Hz70 = 0x0D, Hz60 = 0x0E,
    Hz50 = 0x0F, Hz45 = 0x10, Hz40 = 0x11, Hz35 = 0x12, Hz30 = 0x13, Hz25 = 0x14,
    Hz20 = 0x15, Hz15 = 0x16, Hz10 = 0x17, Hz5 = 0x18, Hz4 = 0x19, Hz3 = 0x1A,
    Hz2 = 0x1B, Hz1 = 0x1C, Hz0_5 = 0x1D, Hz0_25 = 0x1E, Hz0_125 = 0x1F,
};

/// ODR code → nominal rate in Hz (for logging / loop-rate planning).
constexpr float OdrHz(Odr odr) noexcept {
    switch (odr) {
        case Odr::Hz240: return 240.0f;     case Odr::Hz218_5: return 218.5f;
        case Odr::Hz199_1: return 199.1f;   case Odr::Hz179_2: return 179.2f;
        case Odr::Hz160: return 160.0f;     case Odr::Hz149_3: return 149.3f;
        case Odr::Hz140: return 140.0f;     case Odr::Hz129_8: return 129.8f;
        case Odr::Hz120: return 120.0f;     case Odr::Hz110_1: return 110.1f;
        case Odr::Hz100_2: return 100.2f;   case Odr::Hz89_6: return 89.6f;
        case Odr::Hz80: return 80.0f;       case Odr::Hz70: return 70.0f;
        case Odr::Hz60: return 60.0f;       case Odr::Hz50: return 50.0f;
        case Odr::Hz45: return 45.0f;       case Odr::Hz40: return 40.0f;
        case Odr::Hz35: return 35.0f;       case Odr::Hz30: return 30.0f;
        case Odr::Hz25: return 25.0f;       case Odr::Hz20: return 20.0f;
        case Odr::Hz15: return 15.0f;       case Odr::Hz10: return 10.0f;
        case Odr::Hz5: return 5.0f;         case Odr::Hz4: return 4.0f;
        case Odr::Hz3: return 3.0f;         case Odr::Hz2: return 2.0f;
        case Odr::Hz1: return 1.0f;         case Odr::Hz0_5: return 0.5f;
        case Odr::Hz0_25: return 0.25f;     case Odr::Hz0_125: return 0.125f;
    }
    return 0.0f;
}

/// IIR filter coefficient (DSP_IIR).
enum class IirFilter : uint8_t {
    Bypass = 0, Coeff1 = 1, Coeff3 = 2, Coeff7 = 3, Coeff15 = 4, Coeff31 = 5,
    Coeff63 = 6, Coeff127 = 7,
};

/// Interrupt pin behaviour.
enum class IntMode : uint8_t { Pulsed = 0, Latched = 1 };
enum class IntPolarity : uint8_t { ActiveLow = 0, ActiveHigh = 1 };
enum class IntDrive : uint8_t { PushPull = 0, OpenDrain = 1 };

/// Raw → engineering scaling (data sheet §4.x, Bosch API).
constexpr float kPressureLsbPerPa = 64.0f;      ///< pressure_pa = raw / 64
constexpr float kTemperatureLsbPerC = 65536.0f; ///< temperature_c = raw / 65536

}  // namespace bmp581
