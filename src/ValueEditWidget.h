#pragma once

#include "EditWidget.h"
#include "Clock.h"

class ValueEditWidget : public EditWidget
{
public:
	ValueEditWidget(JLMBackBuffer& gfx, const char* suffix);

    void setRange(float min, float max);
	void setDecimals(uint8_t decimals);

    void setValue(float duration);
    float getValue() const;

    int16_t getWidth() const;

    Result process(RotaryEncoder& knob) override;
    void render();

private:
    void _setValue(float duration);
	float m_value = 0;
	float m_min = -999999999.f;
	float m_max = 999999999.f;
	uint8_t m_decimals = 3;
};
