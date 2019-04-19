#include "State.h"
#include "CalibrationState.h"
#include "MeasurementState.h"
#include "Adafruit_GFX.h"

extern GFXcanvas16 s_canvas;

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