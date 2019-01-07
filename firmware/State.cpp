#include "State.h"
#include "CalibrationState.h"
#include "MeasurementState.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"

extern Adafruit_SSD1351 s_display;

static State s_state = State::Measurement;
static bool s_firstTime = true;

void setState(State state)
{
  if (state == s_state)
  {
    return;
  }

  if (!s_firstTime)
  {
    if (s_state == State::Measurement)
    {
      endMeasurementState();
    }
    else if (s_state == State::Calibration)
    {
      endCalibrationState();
    }
  }
  s_firstTime = false;
  s_display.fillScreen(0x0);
  s_display.fillRect(0, 0, s_display.width(), 9, 0xFFFF);

  s_state = state;
  if (s_state == State::Measurement)
  {
    beginMeasurementState();
  }
  else if (s_state == State::Calibration)
  {
    beginCalibrationState();
  }
}


void processState()
{
  if (s_state == State::Measurement)
  {
    processMeasurementState();
  }
  else if (s_state == State::Calibration)
  {
    processCalibrationState();
  }
}