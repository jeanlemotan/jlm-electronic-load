#pragma once

#include <stdint.h>

constexpr uint32_t k_dacResolution = 16;
constexpr uint32_t k_dacPrecision = 1 << k_dacResolution;


void pwmInit();
void setDACPWM(float dac);
