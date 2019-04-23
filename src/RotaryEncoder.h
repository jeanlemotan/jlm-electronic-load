#pragma once

#include <stdint.h>

class RotaryEncoder 
{
public: 
    virtual ~RotaryEncoder() = default;

    enum class ButtonState
    {
        Down,
        Up,
        Pressed,
        Released,
        Disabled,
    };

	virtual void setBoundaries(int16_t minValue, int16_t maxValue, bool wrapValues) = 0;
	virtual void reset(int16_t newValue = 0) = 0;
	virtual void enable() = 0;
	virtual void disable() = 0;
	virtual int16_t encoderDelta() = 0;
	virtual int16_t encoderDeltaAcc() = 0;
	virtual ButtonState buttonState() = 0;
};

