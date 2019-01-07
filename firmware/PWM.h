#pragma once

#include <stdint.h>

constexpr uint32_t k_dacBits = 18;
constexpr uint32_t k_dacPrecision = 1 << k_dacBits;


void pwmInit();
void setDACPWM(float dac);
