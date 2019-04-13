#include "Measurement.h"
#include "Settings.h"
#include "ADS1115.h"
#include "DAC8571.h"
#include <driver/adc.h>
#include <driver/ledc.h>
#include <algorithm>
#include <mutex>
#include <thread>

#define INPUT             0x01
#define OUTPUT            0x02

extern "C"
{
	void digitalWrite(uint8_t pin, uint8_t val);
	void pinMode(uint8_t pin, uint8_t mode);
}

constexpr uint8_t k_4WireGPIO = 16;
constexpr float k_minTargetResistance = 0.2f;
constexpr float k_maxResistance = 999999999999.f;
constexpr float k_maxTargetPower = 100.f;

constexpr uint8_t k_fanGPIO = 17;
constexpr uint32_t k_fanBits = 12;
constexpr uint32_t k_fanPrecision = 1 << k_fanBits;

enum class ADCMux
{
	Current,
	Voltage
};

struct Measurement::Impl
{
	mutable std::mutex mutex;
	std::thread thread;

	ADS1115 adc;
	DAC8571 dac;
	Settings settings;
	float currentRaw = 0.f;
	float voltageRaw = 0.f;
	float current = 0.f;
	float voltage = 0.f;
	float power = 0.f;
	float resistance = k_maxResistance;
	bool isLoadEnabled = false;
	Clock::time_point loadEnabledTP = Clock::time_point(Clock::duration::zero());
	float targetCurrent = 0.f;
	float targetPower = 0.f;
	float targetResistance = 0.f;
	float dacValue = 0.f;
	float fanValue = 0.f;
	bool is4WireEnabled = false;

	ads1115_pga rangePgas[Settings::k_rangeCount] = { ADS1115_PGA_SIXTEEN, ADS1115_PGA_EIGHT, ADS1115_PGA_FOUR, ADS1115_PGA_TWO, ADS1115_PGA_ONE };
	float rangeLimits[Settings::k_rangeCount] = { 0.256f, 0.512f, 1.024f, 2.048f, 4.096f };

	uint8_t currentRange = 4;
	uint8_t nextCurrentRange = 4;
	bool isCurrentAutoRanging = true;
	uint8_t voltageRange = 4;
	uint8_t nextVoltageRange = 4;
	bool isVoltageAutoRanging = true;

	TrackingMode trackingMode = TrackingMode::CC;


	ADCMux adcMux = ADCMux::Voltage;
	Clock::time_point adcSampleStartedTP = Clock::now();

	float temperatureRaw = 0.f;
	float temperature = 0.f;

	float energy = 0.f;
	float charge = 0.f;
	Clock::time_point energyTP = Clock::now();

	Clock::duration loadTimer = Clock::duration::zero();
	Clock::time_point lastLoadTimerTP = Clock::now();
};

Measurement::Measurement()
	: m_impl(new Impl)
{
}

Measurement::~Measurement()
{
}

void Measurement::init()
{
	ADS1115& adc = m_impl->adc;
  	adc.begin();
  	adc.set_data_rate(ADS1115_DATA_RATE_8_SPS);
  	adc.set_mode(ADS1115_MODE_SINGLE_SHOT);
  	adc.set_mux(ADS1115_MUX_GND_AIN0); //switch to voltage pair
  	adc.set_pga(ADS1115_PGA_SIXTEEN);
  	if (adc.trigger_sample() != 0)
  	{
      	printf("\nADC read trigger failed (ads1115 not connected?)");
  	}

	DAC8571& dac = m_impl->dac;
  	dac.begin();
  	dac.write(0);

  	//init temperature adc
	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_6);

	//4 wire gpio
	pinMode(k_4WireGPIO, OUTPUT); 

	//FAN PWM
	ledc_fade_func_install(0);

	{
		ledc_timer_config_t config;
		config.speed_mode = LEDC_HIGH_SPEED_MODE;
		config.duty_resolution = ledc_timer_bit_t(k_fanBits);
		config.timer_num = LEDC_TIMER_2;
		config.freq_hz = 80000000 / k_fanPrecision;
		ESP_ERROR_CHECK(ledc_timer_config(&config));
	}
	{
		ledc_channel_config_t config;
		config.gpio_num = k_fanGPIO;
		config.speed_mode = LEDC_HIGH_SPEED_MODE;
		config.channel = LEDC_CHANNEL_1;
		config.intr_type = LEDC_INTR_DISABLE;
		config.timer_sel = LEDC_TIMER_2;
		config.duty = 100;
		config.hpoint = k_fanPrecision - 1;
		ESP_ERROR_CHECK(ledc_channel_config(&config));
	}

	m_impl->thread = std::thread([this]() { _threadProc(); });
}

void Measurement::_setFan(float fan)
{
  	m_impl->fanValue = std::max(std::min(fan, 1.f), 0.f);
  	uint32_t duty = uint32_t(m_impl->fanValue * float(k_fanPrecision));
  	ESP_ERROR_CHECK(ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, duty, k_fanPrecision - 1));
}
void Measurement::setFan(float fan)
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
  	_setFan(fan);
}
float Measurement::getFan() const
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
  	return m_impl->fanValue;
}

void Measurement::setSettings(Settings settings)
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	m_impl->settings = settings;
}

uint8_t Measurement::getCurrentRange() const
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	return m_impl->currentRange;
}
void Measurement::setCurrentRange(uint8_t range)
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	m_impl->isCurrentAutoRanging = false;
	m_impl->nextCurrentRange = range;
}
bool Measurement::isCurrentAutoRanging() const
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	return m_impl->isCurrentAutoRanging;
}
void Measurement::setCurrentAutoRanging(bool enabled)
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	m_impl->isCurrentAutoRanging = enabled;
}

uint8_t Measurement::getVoltageRange() const
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	return m_impl->voltageRange;
}
void Measurement::setVoltageRange(uint8_t range)
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	m_impl->isVoltageAutoRanging = false;
	m_impl->nextVoltageRange = range;
}
bool Measurement::isVoltageAutoRanging() const
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	return m_impl->isVoltageAutoRanging;
}
void Measurement::setVoltageAutoRanging(bool enabled)
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	m_impl->isVoltageAutoRanging = enabled;
}
void Measurement::set4WireEnabled(bool enabled)
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	if (m_impl->is4WireEnabled != enabled)
	{
		m_impl->is4WireEnabled = enabled;
		digitalWrite(k_4WireGPIO, m_impl->is4WireEnabled);
	}
}
bool Measurement::is4WireEnabled() const
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	return m_impl->is4WireEnabled;
}
void Measurement::_getCurrentBiasScale(float& bias, float& scale) const
{
	bias = m_impl->settings.currentRangeBiases[m_impl->currentRange];
	scale = m_impl->settings.currentRangeScales[m_impl->currentRange];
}
float Measurement::_computeCurrent(float raw) const
{
	float bias, scale;
	_getCurrentBiasScale(bias, scale);
	return (raw + bias) * scale;
}
float Measurement::getCurrent() const
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	return m_impl->current;
}
float Measurement::getCurrentRaw() const
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	return m_impl->currentRaw;
}
void Measurement::_getVoltageBiasScale(float& bias, float& scale) const
{
	bias = m_impl->settings.voltageRangeBiases[m_impl->voltageRange];
	scale = m_impl->settings.voltageRangeScales[m_impl->voltageRange];
}
float Measurement::_computeVoltage(float raw) const
{
	float bias, scale;
	_getVoltageBiasScale(bias, scale);
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
float Measurement::getVoltage() const
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	return m_impl->voltage;
}
float Measurement::getVoltageRaw() const
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	return m_impl->voltageRaw;
}
float Measurement::_computeTemperature(float raw) const
{
	//https://learn.adafruit.com/thermistor/using-a-thermistor

	//return (raw + m_impl->settings.temperatureBias) * m_impl->settings.temperatureScale;
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
float Measurement::getTemperature() const
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	return m_impl->temperature;
}
float Measurement::getTemperatureRaw() const
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	return m_impl->temperatureRaw;
}
float Measurement::getPower() const
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	return m_impl->power;
}
float Measurement::getResistance() const
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	return m_impl->resistance;
}
float Measurement::getEnergy() const
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	return m_impl->energy;
}
float Measurement::getCharge() const
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	return m_impl->charge;
}
void Measurement::resetEnergy()
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	m_impl->energy = 0;
	m_impl->charge = 0;
}
void Measurement::setTargetCurrent(float target)
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	/*
	m_impl->targetCurrent = std::max(std::min(target, k_maxCurrent), 0.f);

	float scale = 0.138969697f;
	float bias = 0.007636364f;

	//float desiredCurrent = (dac * 3.3f / float(k_dacPrecision) + bias) * scale;

	m_impl->dac = m_impl->targetCurrent * scale + bias;
	setDACPWM(m_impl->dac);
	*/
	m_impl->targetCurrent = std::max(std::min(target, k_maxCurrent), 0.f);
	for (size_t i = 1; i < m_impl->settings.dac2CurrentTable.size(); i++)
	{
		float c1 = m_impl->settings.dac2CurrentTable[i];
		if (c1 >= m_impl->targetCurrent)
		{
			float c0 = m_impl->settings.dac2CurrentTable[i - 1];
			float delta = c1 - c0;
			float mu = 0.f;
			if (delta > 0)
			{
				mu = (m_impl->targetCurrent - c0) / delta;
			}

			float szf = float(m_impl->settings.dac2CurrentTable.size());
			float p0 = float(i - 1) / szf;
			p0 = p0 * p0 * p0;
			float p1 = float(i) / szf;
			p1 = p1 * p1 * p1;

			float dac = mu * (p1 - p0) + p0;
			_setDAC(dac);
			//ESP_LOGI("XXX", "c0 %f, c1 %f, delta %f, mu %f, p0 %f, p1 %f, dac %f", c0, c1, delta, mu, p0, p1, dac);
			break;
		}
	}
	//setDAC(0.f);
}
float Measurement::getTargetCurrent() const
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	return m_impl->targetCurrent;
}
void Measurement::setTargetPower(float target)
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	m_impl->targetPower = std::min(target, k_maxTargetPower);
}
float Measurement::getTargetPower() const
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	return m_impl->targetPower;
}
void Measurement::setTargetResistance(float target)
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	m_impl->targetResistance = std::max(target, k_minTargetResistance);
}
float Measurement::getTargetResistance() const
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	return m_impl->targetResistance;
}
void Measurement::setDAC(float dac)
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	_setDAC(dac);
}
void Measurement::_setDAC(float dac)
{
	if (m_impl->isLoadEnabled)
	{
		dac = std::max(std::min(dac, 1.f), 0.f);
	}
	else
	{
		dac = 0;
	}

	if (m_impl->dacValue != dac)
	{
		m_impl->dacValue = dac;
		m_impl->dac.write(uint16_t(dac * 65535.f));
	}
}
float Measurement::getDAC() const
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	return m_impl->dacValue;
}
void Measurement::setLoadEnabled(bool enabled)
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	m_impl->isLoadEnabled = enabled;
	m_impl->loadEnabledTP = Clock::now();
	_setDAC(m_impl->dacValue);
}
bool Measurement::isLoadEnabled() const
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	return m_impl->isLoadEnabled;
}
bool Measurement::isLoadSettled() const
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	//return std::abs(m_impl->targetCurrent - m_impl->current) < m_impl->targetCurrent * 0.05f;
	return Clock::now() - m_impl->loadEnabledTP >= std::chrono::milliseconds(200);
}
Clock::duration Measurement::getLoadTimer() const
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	return m_impl->loadTimer;
}
void Measurement::resetLoadTimer()
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	m_impl->loadTimer = Clock::duration::zero();
}
void Measurement::readAdcs()
{
	bool hasVoltage, hasCurrent, hasTemperature;
	readAdcs(hasVoltage, hasCurrent, hasTemperature);
}
void Measurement::readAdcs(bool& hasVoltage, bool& hasCurrent, bool& hasTemperature)
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);

	ADS1115& adc = m_impl->adc;

	hasVoltage = false;
	hasCurrent = false;
	hasTemperature = false;

	Clock::time_point now = Clock::now();
	if (!adc.is_sample_in_progress() && now >= m_impl->adcSampleStartedTP + std::chrono::milliseconds(130))
	{
		float val = adc.read_sample_float();
		if (m_impl->adcMux == ADCMux::Voltage)
		{
			bool skipSample = false;
			if (m_impl->isVoltageAutoRanging)
			{
				for (size_t i = 0; i < Settings::k_rangeCount; i++)
				{
					float limit = m_impl->rangeLimits[i];
					if (val < limit * 0.9f || i + 1 == Settings::k_rangeCount)
					{
						//ESP_LOGI("XXX", "raw %f, limit %f, limit9 %f, r %d", val, limit, limit*0.9f, (int)i);
						m_impl->nextVoltageRange = i;
						break;
					}
				}
				skipSample = m_impl->voltageRange != m_impl->nextVoltageRange; //if auto ranging, skip sample as it's probably out of date
			}

			if (!skipSample)
			{
				m_impl->voltageRaw = val;
				m_impl->voltage = _computeVoltage(m_impl->voltageRaw);
				if (m_impl->voltageRange == m_impl->nextVoltageRange)
				{
					hasVoltage = true; //don't report values when switching range
				}
			}

			m_impl->currentRange = m_impl->nextCurrentRange;
			adc.set_mux(ADS1115_MUX_GND_AIN3); //switch to current pair
			adc.set_pga(m_impl->rangePgas[m_impl->currentRange]);
			adc.trigger_sample();
			m_impl->adcMux = ADCMux::Current;
		}
		else
		{
			bool skipSample = false;
			if (m_impl->isCurrentAutoRanging)
			{
				for (size_t i = 0; i < Settings::k_rangeCount; i++)
				{
					float limit = m_impl->rangeLimits[i];
					if (val < limit * 0.9f || i + 1 == Settings::k_rangeCount)
					{
						m_impl->nextCurrentRange = i;
						break;
					}
				}
				skipSample = m_impl->currentRange != m_impl->nextCurrentRange; //if auto ranging, skip sample as it's probably out of date
			}

			if (!skipSample)
			{
				m_impl->currentRaw = val;
				m_impl->current = _computeCurrent(m_impl->currentRaw);
				if (m_impl->currentRange == m_impl->nextCurrentRange)
				{
					hasCurrent = true; //don't report values when switching range
				}
			}

			m_impl->voltageRange = m_impl->nextVoltageRange;
			adc.set_mux(ADS1115_MUX_GND_AIN0); //switch to voltage pair
			adc.set_pga(m_impl->rangePgas[m_impl->voltageRange]);
			adc.trigger_sample();
			m_impl->adcMux = ADCMux::Voltage;
		}
		m_impl->adcSampleStartedTP = now;

		m_impl->power = m_impl->voltage * m_impl->current;
		m_impl->resistance = m_impl->current <= 0.0001f ? k_maxResistance : std::abs(m_impl->voltage / m_impl->current);
	}

	m_impl->temperatureRaw = adc1_get_raw(ADC1_CHANNEL_4) / 4096.f;
	m_impl->temperature = (m_impl->temperature * 90.f + _computeTemperature(m_impl->temperatureRaw) * 10.f) / 100.f;
	hasTemperature = true;
}
Measurement::TrackingMode Measurement::getTrackingMode() const
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	return m_impl->trackingMode;
}
void Measurement::setTrackingMode(TrackingMode mode)
{
	std::lock_guard<std::mutex> lg(m_impl->mutex);
	m_impl->trackingMode = mode;
}
void Measurement::_updateTracking()
{

}
void Measurement::_threadProc()
{
	while (true)
	{
		{
			bool readVoltage, readCurrent, readTemperature;
			readAdcs(readVoltage, readCurrent, readTemperature);

			std::lock_guard<std::mutex> lg(m_impl->mutex);

			bool enabled = m_impl->isLoadEnabled;
			float power = m_impl->power;
			float current = m_impl->current;

			Clock::time_point now = Clock::now();

			//if there is no load, remove any leaking current (usually < 0.001);
			//but don't just set it to zero - I want to see if the leak is too big
			if (!enabled && m_impl->current < 0.002f)
			{
				m_impl->current = 0;
			}

			{
				Clock::duration dt = now > m_impl->lastLoadTimerTP ? now - m_impl->lastLoadTimerTP : Clock::duration::zero();
				m_impl->lastLoadTimerTP = now;
				if (enabled)
				{
					m_impl->loadTimer += dt;
				}
			}

			//accumulate energy
			{
				if (enabled && now > m_impl->energyTP)
				{
					float dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - m_impl->energyTP).count() / 3600.f;
					m_impl->energy += power * dt;
					m_impl->charge += current * dt;
				}
				m_impl->energyTP = now;
			}

			_updateTracking();
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void Measurement::process()
{
	//if (!m_impl->thread)
	{
		//_threadProc();
	}
}