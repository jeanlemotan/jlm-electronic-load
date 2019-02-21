#pragma once
#include <stdint.h>

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

float getPower();
float getEnergy();
void resetEnergy();

void setDAC(float target);
float getDAC();
void setTargetCurrent(float target);
void setLoadEnabled(bool enabled);
bool isLoadEnabled();
bool isLoadSettled();

void initMeasurementState();
void beginMeasurementState();
void endMeasurementState();
void processMeasurementState();


