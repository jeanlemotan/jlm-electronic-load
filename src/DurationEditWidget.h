#pragma once

#include "EditWidget.h"
#include "Clock.h"

class DurationEditWidget : public EditWidget
{
public:
	DurationEditWidget(Adafruit_GFX& gfx, const char* suffix);

    void setRange(Clock::duration min, Clock::duration max);

    void setDuration(Clock::duration duration);
    Clock::duration getDuration() const;

    Result process(RotaryEncoder& knob) override;
    void render();

private:
    void _setDuration(Clock::duration duration);
    Clock::duration m_min;
    Clock::duration m_max;
    Clock::duration m_duration;
};
