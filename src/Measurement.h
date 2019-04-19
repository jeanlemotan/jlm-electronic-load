#pragma once
#include <stdint.h>
#include <memory>
#include "Settings.h"
#include "Clock.h"

class Measurement
{
public:
	Measurement();
	~Measurement();

	static constexpr float k_maxVoltage = 30.f;
	static constexpr float k_maxCurrent = 4.f;
	static constexpr float k_minResistance = 0.2f;
	static constexpr float k_maxResistance = k_maxVoltage / 0.001f; 
	static constexpr float k_infiniteResistance = 999999.f; 
	static constexpr float k_maxPower = 100.f;

	void init();
	void process();

	void setSettings(Settings settings);

	float getCurrent() const;
	float getCurrentRaw() const;
	uint8_t getCurrentRange() const;
	void setCurrentRange(uint8_t range);
	bool isCurrentAutoRanging() const;
	void setCurrentAutoRanging(bool enabled);

	float getVoltage() const;
	float getVoltageRaw() const;
	uint8_t getVoltageRange() const;
	void setVoltageRange(uint8_t range);
	bool isVoltageAutoRanging() const;
	void setVoltageAutoRanging(bool enabled);

	void set4WireEnabled(bool enabled);
	bool is4WireEnabled() const;

	float getTemperature() const;
	float getTemperatureRaw() const;

	float getPower() const;
	float getResistance() const;
	float getEnergy() const;
	float getCharge() const;
	void resetEnergy();

	void setDAC(float dac);
	float getDAC() const;

	void setFan(float fan);
	float getFan() const;

	void setTargetCurrent(float target);
	float getTargetCurrent() const;

	void setTargetResistance(float target);
	float getTargetResistance() const;

	void setTargetPower(float target);
	float getTargetPower() const;

	void setLoadEnabled(bool enabled);
	bool isLoadEnabled() const;
	bool isLoadSettled() const;

	Clock::duration getLoadTimer() const;
	void resetLoadTimer();

	void setVoltageLimit(float limit);
	float getVoltateLimit() const;
	void setVoltageLimitEnabled(bool enabled);
	bool isVoltageLimitEnabled() const;

	void setEnergyLimit(float limit);
	float getEnergyLimit() const;
	void setEnergyLimitEnabled(bool enabled);
	bool isEnergyLimitEnabled() const;

	void setChargeLimit(float limit);
	float getChargeLimit() const;
	void setChargeLimitEnabled(bool enabled);
	bool isChargeLimitEnabled() const;

	void setLoadTimerLimit(Clock::duration limit);
	Clock::duration getLoadTimerLimit() const;
	void setLoadTimerLimitEnabled(bool enabled);
	bool isLoadTimerLimitEnabled() const;

	enum class StopCondition
	{
		None,
		VoltageLimit,
		EnergyLimit,
		ChargeLimit,
		LoadTimerLimit
	};

	StopCondition getStopCondition() const;

	enum class TrackingMode
	{
		None,
		CC, 
		CP, 
		CR
	};
	TrackingMode getTrackingMode() const;
	void setTrackingMode(TrackingMode mode);

private:
	void _readAdcs();
	void _readAdcs(bool& voltage, bool& current, bool& temperature);
	float _computeDACForCurrent(float current) const;
	enum class SPS
	{
		_8,
		_16,
		_32,
		_64
	};
	void _setSPS(SPS sps);

	void _setFan(float fan);
	void _setDAC(float dac);
	void _updateTracking();
	float _computeVoltage(float raw) const;
	void _getVoltageBiasScale(float& bias, float& scale) const;
	float _computeCurrent(float raw) const;
	void _getCurrentBiasScale(float& bias, float& scale) const;
	float _computeTemperature(float raw) const;
	void _checkStopConditions();
	void _threadProc();

	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
