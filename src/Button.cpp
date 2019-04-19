#include "Button.h"

Button::Button(uint8_t pin)
	: m_pin(pin)
{
	pinMode(m_pin, INPUT_PULLUP);
}

Button::State Button::state()
{
	uint8_t buttState = !digitalRead(m_pin);
	if (buttState && !m_previousState)
	{
		m_previousState = true;
		return State::PUSHED;
	}
	if (!buttState && m_previousState)
	{
		m_previousState = false;
		return State::RELEASED; 
	}
	return (buttState ? State::DOWN : State::UP);
}
