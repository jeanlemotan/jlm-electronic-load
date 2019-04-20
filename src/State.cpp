#include "State.h"
#include "CalibrationState.h"
#include "MeasurementState.h"
#include "DeltaBitmap.h"

extern DeltaBitmap s_canvas;

static State s_state = State::None;
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