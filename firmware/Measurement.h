#pragma once
#include <stdint.h>
#include "Clock.h"

constexpr float k_maxCurrent = 4.f;

void readAdcs();
void readAdcs(bool& voltage, bool& current, bool& temperature);

float getCurrent();
float getCurrentRaw();
float getVoltage();
float getVoltageRaw();
float getTemperature();
float getTemperatureRaw();
bool isVoltageValid();

uint8_t getCurrentRange();
void setCurrentRange(uint8_t range);
bool isCurrentAutoRanging();
void setCurrentAutoRanging(bool enabled);

uint8_t getVoltageRange();
void setVoltageRange(uint8_t range);
bool isVoltageAutoRanging();
void setVoltageAutoRanging(bool enabled);

void set4WireEnabled(bool enabled);
bool is4WireEnabled();

float getPower();
float getEnergy();
float getCharge();
void resetEnergy();

void setDAC(float target);
float getDAC();

void setTargetCurrent(float target);
float getTargetCurrent();

void setTargetResistance(float target);
float getTargetResistance();

void setTargetPower(float target);
float getTargetPower();

void setLoadEnabled(bool enabled);
bool isLoadEnabled();
bool isLoadSettled();

Clock::duration getLoadTimer();
void resetLoadTimer();

enum class TrackingMode
{
	CC, 
	CP, 
	CR
};
TrackingMode getTrackingMode();
void setTrackingMode(TrackingMode mode);

void processMeasurement();
void initMeasurement();