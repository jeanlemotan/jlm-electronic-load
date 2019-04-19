#pragma once

#include "Arduino.h"

class Button
{
public:
	enum class State
	{
		DOWN,
		PUSHED,
		UP,
		RELEASED,
	};

	Button(uint8_t pin);
	State state();

private:
	uint8_t m_pin = 0;
	bool m_previousState = false;
};


