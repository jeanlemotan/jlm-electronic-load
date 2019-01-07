#pragma once

enum class State
{
  Measurement,
  Calibration
};
void setState(State state);
void processState();
