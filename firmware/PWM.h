#pragma once

#include <stdint.h>

constexpr uint32_t k_fanBits = 12;
constexpr uint32_t k_fanPrecision = 1 << k_fanBits;

void pwmInit();
void setFanPWM(float dac);
