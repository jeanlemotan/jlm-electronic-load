#pragma once

#include <stdint.h>

constexpr uint32_t k_dacBits = 18;
constexpr uint32_t k_dacPrecision = 1 << k_dacBits;

constexpr uint32_t k_fanBits = 8;
constexpr uint32_t k_fanPrecision = 1 << k_fanBits;

void pwmInit();
void setDACPWM(float dac);
void setFanPWM(float dac);
