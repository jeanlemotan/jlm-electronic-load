#pragma once

enum class State
{
  None,
  Measurement,
  Calibration
};
void setState(State state);
void processState();
