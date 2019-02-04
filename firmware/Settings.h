#pragma once
#include <stdint.h>
#include <array>

struct Settings
{
	static constexpr uint8_t k_version = 0;
	uint8_t version = k_version;
	uint32_t crc = 0;
	float temperatureBias = 0.0f;
	float temperatureScale = 1.0f;

	static constexpr uint8_t k_rangeCount = 5;
	std::array<float, k_rangeCount> currentRangeBiases = {{ 0.f, 0.f, 0.f, 0.f, 0.f }};
	std::array<float, k_rangeCount> currentRangeScales = {{ 1.f, 1.f, 1.f, 1.f, 1.f }};
	std::array<float, k_rangeCount> voltageRangeBiases = {{ 0.f, 0.f, 0.f, 0.f, 0.f }};
	std::array<float, k_rangeCount> voltageRangeScales = {{ 1.f, 1.f, 1.f, 1.f, 1.f }};	
};

bool loadSettings(Settings& settings);
void saveSettings(const Settings& settings);
