#pragma once
#include <stdint.h>

void readAdcs();
float getCurrent();
float getCurrentRaw();
float getVoltage();
float getVoltageRaw();
float getTemperature();
float getTemperatureRaw();
bool isVoltageValid();

uint8_t getCurrentRange();
void setCurrentRange(uint8_t range);
bool isSwitchingCurrentRange();
bool isCurrentAutoRanging();
void setCurrentAutoRanging(bool enabled);

uint8_t getVoltageRange();
void setVoltageRange(uint8_t range);
bool isSwitchingVoltageRange();
bool isVoltageAutoRanging();
void setVoltageAutoRanging(bool enabled);

void initMeasurementState();
void beginMeasurementState();
void endMeasurementState();
void processMeasurementState();


