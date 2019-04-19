#pragma once

#include <stdint.h>

class DAC8571 {
public:
	DAC8571(uint8_t address = 0x4C);

	void begin();
	uint8_t write(uint16_t data);

private:
	uint8_t write_register(uint8_t reg, uint16_t val);
	uint16_t read_register(uint8_t reg);

	uint8_t m_address;
};
