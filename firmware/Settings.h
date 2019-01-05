#pragma once
#include <stdint.h>

static constexpr uint8_t k_rangeCount = 5;

struct Settings
{
  float temperatureBias = 0.0f;
  float temperatureScale = 126.8f;
	float currentRangeBiases[k_rangeCount] = { 0.f, 0.f, -0.016f, 0.f, -0.0167f };
	float currentRangeScales[k_rangeCount] = { 1.f, 1.f, 0.45045045f, 1.f, 0.490464548f };
	float voltageRangeBiases[k_rangeCount] = { 0.f, 0.f, -0.01f, 0.f, 0.f };
	float voltageRangeScales[k_rangeCount] = { 1.f, 1.f, 11.073089701f, 1.f, 1.f };	
};

bool loadSettings(Settings& settings);
void saveSettings(const Settings& settings);
