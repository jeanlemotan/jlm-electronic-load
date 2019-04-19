#pragma once
#include <stdint.h>
#include <array>

struct Settings
{
	Settings();

	bool load();
	void save();

	static constexpr uint8_t k_rangeCount = 5;
	static constexpr uint8_t k_version = 1;
	struct Data
	{
		uint8_t version = k_version;
		uint32_t crc = 0;
		
		float temperatureBias = 0.0f;
		float temperatureScale = 1.0f;
		float dacBias = 0.0f;
		float dacScale = 1.0f;

		std::array<float, k_rangeCount> currentRangeBiases = {{ 0.f, 0.f, 0.f, 0.f, 0.f }};
		std::array<float, k_rangeCount> currentRangeScales = {{ 1.f, 1.f, 1.f, 1.f, 1.f }};
		std::array<float, k_rangeCount> voltageRangeBiases = {{ 0.f, 0.f, 0.f, 0.f, 0.f }};
		std::array<float, k_rangeCount> voltageRangeScales = {{ 1.f, 1.f, 1.f, 1.f, 1.f }};
	} data;
};
