#include "Measurement.h"
#include "Settings.h"
#include "ADS1115.h"
#include "DAC8571.h"
#include "PWM.h"
#include <driver/adc.h>
#include <algorithm>

extern "C"
{
	void digitalWrite(uint8_t pin, uint8_t val);
}

extern Settings s_settings;
extern ADS1115 s_adc;
extern DAC8571 s_dac;
extern uint8_t k_4WirePin;

static const float k_minTargetResistance = 0.2f;
static const float k_maxTargetPower = 100.f;

static float s_currentRaw = 0.f;
static float s_voltageRaw = 0.f;
static float s_current = 0.f;
static float s_voltage = 0.f;
static bool s_loadEnabled = false;
static Clock::time_point s_loadEnabledTP = Clock::time_point(Clock::duration::zero());
static float s_targetCurrent = 0.f;
static float s_targetPower = 0.f;
static float s_targetResistance = 0.f;
static float s_dacValue = 0.f;
static bool s_4WireEnabled = false;

ads1115_pga s_rangePgas[Settings::k_rangeCount] = { ADS1115_PGA_SIXTEEN, ADS1115_PGA_EIGHT, ADS1115_PGA_FOUR, ADS1115_PGA_TWO, ADS1115_PGA_ONE };
float s_rangeLimits[Settings::k_rangeCount] = { 0.256f, 0.512f, 1.024f, 2.048f, 4.096f };

static uint8_t s_currentRange = 4;
static uint8_t s_nextCurrentRange = 4;
static bool s_isCurrentAutoRanging = true;
static uint8_t s_voltageRange = 4;
static uint8_t s_nextVoltageRange = 4;
static bool s_isVoltageAutoRanging = true;

static TrackingMode s_trackingMode = TrackingMode::CC;

enum class ADCMux
{
	Current,
	Voltage
};

static ADCMux s_adcMux = ADCMux::Voltage;
static Clock::time_point s_adcSampleStartedTP = Clock::now();

static float s_temperatureRaw = 0.f;
static float s_temperature = 0.f;

static float s_energy = 0.f;
static float s_charge = 0.f;
static Clock::time_point s_energyTP = Clock::now();

static Clock::duration s_loadTimer = Clock::duration::zero();
static Clock::time_point s_lastLoadTimerTP = Clock::now();

uint8_t getCurrentRange()
{
	return s_currentRange;
}
void setCurrentRange(uint8_t range)
{
	s_isCurrentAutoRanging = false;
	s_nextCurrentRange = range;
}
bool isCurrentAutoRanging()
{
	return s_isCurrentAutoRanging;
}
void setCurrentAutoRanging(bool enabled)
{
	s_isCurrentAutoRanging = enabled;
}

uint8_t getVoltageRange()
{
	return s_voltageRange;
}
void setVoltageRange(uint8_t range)
{
	s_isVoltageAutoRanging = false;
	s_nextVoltageRange = range;
}
bool isVoltageAutoRanging()
{
	return s_isVoltageAutoRanging;
}
void setVoltageAutoRanging(bool enabled)
{
	s_isVoltageAutoRanging = enabled;
}

void set4WireEnabled(bool enabled)
{
	if (s_4WireEnabled != enabled)
	{
		s_4WireEnabled = enabled;
		digitalWrite(k_4WirePin, s_4WireEnabled);
	}
}
bool is4WireEnabled()
{
	return s_4WireEnabled;
}

void getCurrentBiasScale(float& bias, float& scale)
{
	bias = s_settings.currentRangeBiases[s_currentRange];
	scale = s_settings.currentRangeScales[s_currentRange];
}
float computeCurrent(float raw)
{
	float bias, scale;
	getCurrentBiasScale(bias, scale);
	return (raw + bias) * scale;
}
float getCurrent()
{
	return s_current;
}
float getCurrentRaw()
{
	return s_currentRaw;
}

void getVoltageBiasScale(float& bias, float& scale)
{
	bias = s_settings.voltageRangeBiases[s_voltageRange];
	scale = s_settings.voltageRangeScales[s_voltageRange];
}
float computeVoltage(float raw)
{
	float bias, scale;
	getVoltageBiasScale(bias, scale);
/*	Serial.print("raw ");
	Serial.print(raw, 4);
	Serial.print(", bias ");
	Serial.print(bias, 4);
	Serial.print(", scale ");
	Serial.print(scale, 4);
	Serial.print(", v ");
	Serial.println((raw + bias) * scale);
*/	return (raw + bias) * scale;
}
float getVoltage()
{
	return s_voltage;
}
float getVoltageRaw()
{
	return s_voltageRaw;
}

float computeTemperature(float raw)
{
	//https://learn.adafruit.com/thermistor/using-a-thermistor

	//return (raw + s_settings.temperatureBias) * s_settings.temperatureScale;
	//adc = R / (R + 10K) * Vcc / Vref
	float vcc = 3.3f;
	float vref = 2.2f; //the adc attenuation
	float R = (10000.f * raw) / (vcc / vref - raw);

	// 1/T = 1/T0 + 1/B * ln(R/R0);
	float T0 = 298.15f; //25 degrees celsius
	float B = 3950; //thermistor coefficient
	float R0 = 10000.f; //thermistor resistance at room temp

	float Tinv = 1.f/T0 + 1.f/B * log(R/R0);
	float T = 1.f / Tinv;

	float Tc = T - 273.15f;

	return Tc;
}
float getTemperature()
{
	return s_temperature;
}
float getTemperatureRaw()
{
	return s_temperatureRaw;
}

float getPower()
{
	return getVoltage() * getCurrent();
}

float getEnergy()
{
	return s_energy;
}
float getCharge()
{
	return s_charge;
}

void resetEnergy()
{
	s_energy = 0;
	s_charge = 0;
}

void setTargetCurrent(float target)
{
	/*
	s_targetCurrent = std::max(std::min(target, k_maxCurrent), 0.f);

	float scale = 0.138969697f;
	float bias = 0.007636364f;

	//float desiredCurrent = (dac * 3.3f / float(k_dacPrecision) + bias) * scale;

	s_dac = s_targetCurrent * scale + bias;
	setDACPWM(s_dac);
	*/
	s_targetCurrent = std::max(std::min(target, k_maxCurrent), 0.f);
	for (size_t i = 1; i < s_settings.dac2CurrentTable.size(); i++)
	{
		float c1 = s_settings.dac2CurrentTable[i];
		if (c1 >= s_targetCurrent)
		{
			float c0 = s_settings.dac2CurrentTable[i - 1];
			float delta = c1 - c0;
			float mu = 0.f;
			if (delta > 0)
			{
				mu = (s_targetCurrent - c0) / delta;
			}

			float szf = float(s_settings.dac2CurrentTable.size());
			float p0 = float(i - 1) / szf;
			p0 = p0 * p0 * p0;
			float p1 = float(i) / szf;
			p1 = p1 * p1 * p1;

			float dac = mu * (p1 - p0) + p0;
			setDAC(dac);
			//ESP_LOGI("XXX", "c0 %f, c1 %f, delta %f, mu %f, p0 %f, p1 %f, dac %f", c0, c1, delta, mu, p0, p1, dac);
			break;
		}
	}
	//setDAC(0.f);
}
float getTargetCurrent()
{
	return s_targetCurrent;
}
void setTargetPower(float target)
{
	s_targetPower = std::min(target, k_maxTargetPower);
}
float getTargetPower()
{
	return s_targetPower;
}
void setTargetResistance(float target)
{
	s_targetResistance = std::max(target, k_minTargetResistance);
}
float getTargetResistance()
{
	return s_targetResistance;
}

void setDAC(float dac)
{
	s_dacValue = std::max(std::min(dac, 1.f), 0.f);
	if (s_loadEnabled)
	{
		s_dac.write(uint16_t(s_dacValue * 65535.f));
	}
	else
	{
		s_dac.write(0);
	}
}

float getDAC()
{
	return s_dacValue;
}

void setLoadEnabled(bool enabled)
{
	s_loadEnabled = enabled;
	s_loadEnabledTP = Clock::now();
	setDAC(s_dacValue);
}
bool isLoadEnabled()
{
	return s_loadEnabled;
}
bool isLoadSettled()
{
	//return std::abs(s_targetCurrent - s_current) < s_targetCurrent * 0.05f;
	return Clock::now() - s_loadEnabledTP >= std::chrono::milliseconds(200);
}
Clock::duration getLoadTimer()
{
	return s_loadTimer;
}
void resetLoadTimer()
{
	s_loadTimer = Clock::duration::zero();
}


void readAdcs()
{
	bool voltage, current, temperature;
	readAdcs(voltage, current, temperature);
}

void readAdcs(bool& voltage, bool& current, bool& temperature)
{
	voltage = false;
	current = false;
	temperature = false;

	Clock::time_point now = Clock::now();
	if (!s_adc.is_sample_in_progress() && now >= s_adcSampleStartedTP + std::chrono::milliseconds(130))
	{
		float val = s_adc.read_sample_float();
		if (s_adcMux == ADCMux::Voltage)
		{
			bool skipSample = false;
			if (s_isVoltageAutoRanging)
			{
				for (size_t i = 0; i < Settings::k_rangeCount; i++)
				{
					float limit = s_rangeLimits[i];
					if (val < limit * 0.9f || i + 1 == Settings::k_rangeCount)
					{
						//ESP_LOGI("XXX", "raw %f, limit %f, limit9 %f, r %d", val, limit, limit*0.9f, (int)i);
						s_nextVoltageRange = i;
						break;
					}
				}
				skipSample = s_voltageRange != s_nextVoltageRange; //if auto ranging, skip sample as it's probably out of date
			}

			if (!skipSample)
			{
				s_voltageRaw = val;
				s_voltage = computeVoltage(s_voltageRaw);
				if (s_voltageRange == s_nextVoltageRange)
				{
					voltage = true; //don't report values when switching range
				}
			}

			s_currentRange = s_nextCurrentRange;
			s_adc.set_mux(ADS1115_MUX_GND_AIN3); //switch to current pair
			s_adc.set_pga(s_rangePgas[s_currentRange]);
			s_adc.trigger_sample();
			s_adcMux = ADCMux::Current;
		}
		else
		{
			bool skipSample = false;
			if (s_isCurrentAutoRanging)
			{
				for (size_t i = 0; i < Settings::k_rangeCount; i++)
				{
					float limit = s_rangeLimits[i];
					if (val < limit * 0.9f || i + 1 == Settings::k_rangeCount)
					{
						s_nextCurrentRange = i;
						break;
					}
				}
				skipSample = s_currentRange != s_nextCurrentRange; //if auto ranging, skip sample as it's probably out of date
			}

			if (!skipSample)
			{
				s_currentRaw = val;
				s_current = computeCurrent(s_currentRaw);
				if (s_currentRange == s_nextCurrentRange)
				{
					current = true; //don't report values when switching range
				}
			}

			s_voltageRange = s_nextVoltageRange;
			s_adc.set_mux(ADS1115_MUX_GND_AIN0); //switch to voltage pair
			s_adc.set_pga(s_rangePgas[s_voltageRange]);
			s_adc.trigger_sample();
			s_adcMux = ADCMux::Voltage;
		}
		s_adcSampleStartedTP = now;
	}

	//accumulate energy
	{
		float power = getCurrent() * getVoltage();
		if (now > s_energyTP && isLoadEnabled())
		{
			float dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - s_energyTP).count() / 3600.f;
			s_energy += power * dt;
			s_charge += getCurrent() * dt;
		}
		s_energyTP = now;
	}

	s_temperatureRaw = adc1_get_raw(ADC1_CHANNEL_4) / 4096.f;
	s_temperature = (s_temperature * 90.f + computeTemperature(s_temperatureRaw) * 10.f) / 100.f;
	temperature = true;
}

bool isVoltageValid()
{
	return s_voltage > -0.1f;
}

TrackingMode getTrackingMode()
{
	return s_trackingMode;
}
void setTrackingMode(TrackingMode mode)
{
	s_trackingMode = mode;
}

void updateTracking()
{

}

void processMeasurement()
{
	readAdcs();

	//if there is no load, remove any leaking current (usually < 0.001);
	//but don't just set it to zero - I want to see if the leak is too big
	if (!isLoadEnabled() && s_current < 0.002f)
	{
		s_current = 0;
	}

	{
		Clock::time_point now = Clock::now();
		Clock::duration dt = now > s_lastLoadTimerTP ? now - s_lastLoadTimerTP : Clock::duration::zero();
		s_lastLoadTimerTP = now;
		if (isLoadEnabled())
		{
			s_loadTimer += dt;
		}
	}

	updateTracking();
}

