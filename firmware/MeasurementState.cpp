#include "MeasurementState.h"
#include "ValueWidget.h"
#include "LabelWidget.h"
#include "GraphWidget.h"
#include "Settings.h"
#include "ADS1115.h"
#include "DAC8571.h"
#include "Adafruit_GFX.h"
#include "AiEsp32RotaryEncoder.h"
#include "PWM.h"
#include "Button.h"
#include <driver/adc.h>
#include "Program.h"
#include "Menu.h"
#include "Fonts/SansSerif_plain_10.h"
#include "Fonts/SansSerif_bold_10.h"
#include "Fonts/SansSerif_plain_13.h"
#include "Fonts/SansSerif_bold_13.h"
#include "Fonts/SansSerif_plain_18.h"
#include "Fonts/SansSerif_bold_18.h"
#include "Fonts/SansSerif_plain_28.h"
#include "Fonts/SansSerif_bold_28.h"
#include "Fonts/SansSerif_plain_32.h"
#include "Fonts/SansSerif_bold_32.h"

extern GFXcanvas16 s_canvas;
extern int16_t s_windowY;
extern AiEsp32RotaryEncoder s_knob;
extern Button s_button;
extern ADS1115 s_adc;
extern DAC8571 s_dac;
extern Settings s_settings;
extern uint8_t k_4WirePin;

extern LabelWidget s_modeWidget;
//static LabelWidget s_rangeWidget(s_canvas, "");
static ValueWidget s_voltageWidget(s_canvas, 0.f, "V");
static ValueWidget s_currentWidget(s_canvas, 0.f, "A");
static ValueWidget s_resistanceWidget(s_canvas, 0.f, "{"); //Create the ohm symbol with this: https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
static ValueWidget s_powerWidget(s_canvas, 0.f, "W");
static ValueWidget s_energyWidget(s_canvas, 0.f, "Wh");
static ValueWidget s_chargeWidget(s_canvas, 0.f, "Ah");
static LabelWidget s_timerWidget(s_canvas, "00:00:00");
static GraphWidget s_graphWidget(s_canvas, 512);



static LabelWidget s_targetLabelWidget(s_canvas, "Target:");
static ValueWidget s_targetWidget(s_canvas, 0.f, "Ah");

static uint16_t k_voltageColor = 0x05D2;
static uint16_t k_currentColor = 0x0C3C;
static uint16_t k_resistanceColor = 0xFE4D;
static uint16_t k_powerColor = 0xFBAE;
static uint16_t k_timerColor = 0xFFFF;
static uint16_t k_chargeColor = 0x6AFC;
static uint16_t k_energyColor = 0xD186;

static uint8_t s_voltagePlot = 0;
static uint8_t s_currentPlot = 0;
static uint8_t s_powerPlot = 0;
static uint8_t s_resistancePlot = 0;
static uint8_t s_energyPlot = 0;
static uint8_t s_chargePlot = 0;

static Menu s_menu;
enum class MenuSection
{
	Disabled,
	Main,
	SetTarget,
	ProgramSelection
};
static MenuSection s_menuSection = MenuSection::Disabled;
static size_t s_menuSelection = 0;

static float s_currentRaw = 0.f;
static float s_voltageRaw = 0.f;
static float s_current = 0.f;
static float s_voltage = 0.f;
static bool s_loadEnabled = false;
static uint32_t s_loadEnabledTP = 0;
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

enum class Mode
{
	CC, 
	CP, 
	CR
};
static Mode s_mode = Mode::CC;

enum class ADCMux
{
	Current,
	Voltage
};

static ADCMux s_adcMux = ADCMux::Voltage;
static uint32_t s_adcSampleStartedTP = millis();

static float s_temperatureRaw = 0.f;
static float s_temperature = 0.f;

static float s_energy = 0.f;
static float s_charge = 0.f;
static uint32_t s_energyTP = 0;

static uint64_t s_timer = 0;
static uint32_t s_lastTimerTP = 0;

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
	s_loadEnabledTP = millis();
	setDAC(s_dacValue);
}
bool isLoadEnabled()
{
	return s_loadEnabled;
}
bool isLoadSettled()
{
	return fabs(s_targetCurrent - s_current) < s_targetCurrent * 0.05f;
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

	if (!s_adc.is_sample_in_progress() && millis() >= s_adcSampleStartedTP + 130)
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
		s_adcSampleStartedTP = millis();
	}

	//accumulate energy
	{
		float power = getCurrent() * getVoltage();
		uint32_t tp = millis();
		if (tp > s_energyTP && isLoadEnabled())
		{
			float dt = (tp - s_energyTP) / (3600.f * 1000.f);
			s_energy += power * dt;
			s_charge += getCurrent() * dt;
		}
		s_energyTP = tp;
	}

	s_temperatureRaw = adc1_get_raw(ADC1_CHANNEL_4) / 4096.f;
	s_temperature = (s_temperature * 90.f + computeTemperature(s_temperatureRaw) * 10.f) / 100.f;
	temperature = true;
}

bool isVoltageValid()
{
	return s_voltage > -0.1f;
}

void updateTracking()
{

}

void printOutput()
{
	static float voltageAcc = 0;
	static float currentAcc = 0;
	static size_t sampleCount = 0;

	voltageAcc += getVoltage();
	currentAcc += getCurrent();
	sampleCount++;

	static int lastTP = millis();
	if (millis() < lastTP + 1000)
	{
		return;
	}
	lastTP = millis();

	static uint32_t sampleIndex = 0;
	Serial.print(sampleIndex);
	Serial.print(", ");
	Serial.print(voltageAcc / float(sampleCount), 5);
	Serial.print(", ");
	Serial.print(currentAcc / float(sampleCount), 5);
	Serial.print(", ");
	Serial.print(getEnergy(), 5);
	Serial.print(", ");
	Serial.print(getCharge(), 5);
	Serial.print(", ");
	Serial.print(getTemperature());
	Serial.println("");

	sampleCount = 0;
	voltageAcc = 0;
	currentAcc = 0;

	sampleIndex++;
}

static int32_t s_targetCurrent_mAh = 0;

static void refreshSubMenu()
{
	char buf[32];

	if (s_menuSection == MenuSection::Main)
	{
		sprintf(buf, "Target: %.3f %s", s_targetCurrent, "A");
		s_menu.getSubMenuEntry(1).text = buf;
	}
	else if (s_menuSection == MenuSection::SetTarget)
	{
		sprintf(buf, "Target:> %.3f %s", s_targetCurrent_mAh / 1000.f, "A");
		s_menu.getSubMenuEntry(1).text = buf;
	}
	sprintf(buf, "4 Wire: %s", s_4WireEnabled ? "On" : "Off");
	s_menu.getSubMenuEntry(2) = buf;

	sprintf(buf, "Mode: %s", s_mode == Mode::CC ? "CC" : s_mode == Mode::CP ? "CP" : "CR");
	s_menu.getSubMenuEntry(3) = buf;
}

void setUnitValue(ValueWidget& widget, float value, uint8_t decimalsMacro, float maxMacro, uint8_t decimalsMicro, float maxMicro, const char* unitSI)
{
	char buf[8];
	if (value >= 1000000.f)
	{
		widget.setValue(value / 1000000.f);
		widget.setDecimals(decimalsMacro);
		widget.setLimits(0, maxMacro);
		sprintf(buf, "M%s", unitSI);
		widget.setSuffix(buf);
	}
	else if (value >= 1000.f)
	{
		widget.setValue(value / 1000.f);
		widget.setDecimals(decimalsMacro);
		widget.setLimits(0, maxMacro);
		sprintf(buf, "k%s", unitSI);
		widget.setSuffix(buf);
	}
	else if (value >= 1.f)
	{
		widget.setValue(value);
		widget.setDecimals(decimalsMacro);
		widget.setLimits(0, maxMacro);
		widget.setSuffix(unitSI);
	}	
	else
	{
		widget.setValue(value * 1000.f);
		widget.setDecimals(decimalsMicro);
		widget.setLimits(0, maxMicro);
		sprintf(buf, "m%s", unitSI);
		widget.setSuffix(buf);
	}	
}

void processMeasurementState()
{
	readAdcs();

	if (s_button.state() == Button::State::RELEASED)
	{
		setLoadEnabled(!isLoadEnabled());
	}

	if (s_menuSection == MenuSection::Main)
	{
		refreshSubMenu();
		size_t selection = s_menu.process(s_knob);
		if (selection == 0)
		{
			s_menuSection = MenuSection::Disabled;
		}
		else if (selection == 1)
		{
			s_menuSection = MenuSection::SetTarget;
		}
		else if (selection == 2)
		{
			s_4WireEnabled = !s_4WireEnabled;
			digitalWrite(k_4WirePin, s_4WireEnabled);
		}
		else if (selection == 3)
		{
			s_mode = s_mode == Mode::CC ? Mode::CP : s_mode == Mode::CP ? Mode::CR : Mode::CC;
		}
		else if (selection == 4)
		{
			resetEnergy();
		}
		else if (selection == 6)
		{
			stopProgram();
		}
	}
	else if (s_menuSection == MenuSection::SetTarget)
	{
		int knobDelta = s_knob.encoderChangedAcc();
		s_targetCurrent_mAh += knobDelta;
		s_targetCurrent_mAh = std::max(std::min(s_targetCurrent_mAh, int32_t(k_maxCurrent * 1000.f)), 0);

		if (s_knob.currentButtonState() == BUT_RELEASED)
		{
			setTargetCurrent(s_targetCurrent_mAh / 1000.f);
			s_menuSection = MenuSection::Main;
		}

		refreshSubMenu();
	}
	else if (s_menuSection == MenuSection::Disabled)
	{
		if (s_knob.currentButtonState() == BUT_RELEASED)
		{
			s_menuSection = MenuSection::Main;
		}
	}


	//if there is no load, remove any leaking current (usually < 0.001);
	//but don't just set it to zero - I want to see if the leak is too big
	if (!isLoadEnabled() && s_current < 0.002f)
	{
		s_current = 0;
	}

	//Mode
	s_modeWidget.setValue(isRunningProgram() ? "Program Mode" : s_mode == Mode::CC ? "CC Mode" : s_mode == Mode::CP ? "CP Mode" : "CR Mode");
	s_modeWidget.render();

	//char buf[32];
	//sprintf(buf, "(V%d/A%d)", getVoltageRange(), getCurrentRange());
	//s_rangeWidget.setValue(buf);
  	//s_rangeWidget.setPosition(Widget::Position{s_canvas.width() - s_rangeWidget.getWidth() - 2, s_windowY + s_rangeWidget.getHeight()});
	//s_rangeWidget.render();

	//s_voltageWidget.setValueColor(isVoltageValid() ? 0xFFFF : 0xF000);
	//s_voltageWidget.setDecimals(s_voltage < 10.f ? 3 : 2);
	setUnitValue(s_voltageWidget, s_voltage, 3, 99.f, 1, 999.999f, "V");
	s_voltageWidget.render();

	uint16_t trackedColor = 0;
	int16_t trackedBorderY = 0;

	if (s_mode == Mode::CC)
	{
		int16_t border = 3;
		Widget::Position p = s_currentWidget.getPosition(Widget::Anchor::TopLeft);
		s_canvas.fillRoundRect(p.x - border, p.y - border, 1000, s_currentWidget.getHeight() + border*2, border, k_currentColor);
		//s_canvas.fillRect(p.x, p.y, s_currentWidget.getWidth(), s_currentWidget.getHeight(), k_currentColor),
		s_currentWidget.setTextColor(0x0);
		setUnitValue(s_targetWidget, s_targetCurrent, 3, 99.f, 1, 999.999f, "A");
		trackedColor = k_currentColor;
		trackedBorderY = p.y;
	}
	else
	{
		s_currentWidget.setTextColor(k_currentColor);
	} 	
	setUnitValue(s_currentWidget, s_current, 3, 99.f, 1, 999.999f, "A");
	s_currentWidget.render();

	if (s_mode == Mode::CP)
	{
		int16_t border = 3;
		Widget::Position p = s_powerWidget.getPosition(Widget::Anchor::TopLeft);
		s_canvas.fillRoundRect(p.x - border, p.y - border, 1000, s_powerWidget.getHeight() + border*2, border, k_powerColor);
		s_powerWidget.setTextColor(0x0);
		setUnitValue(s_targetWidget, s_targetPower, 3, 999.999f, 1, 999.999f, "W");
		trackedColor = k_powerColor;
		trackedBorderY = p.y;
	}
	else
	{
		s_powerWidget.setTextColor(k_powerColor);
	}
	float power = std::abs(s_voltage * s_current);
	setUnitValue(s_powerWidget, power, 3, 999.999f, 1, 999.999f, "W");
	s_powerWidget.render();

	if (s_mode == Mode::CR)
	{
		int16_t border = 3;
		Widget::Position p = s_resistanceWidget.getPosition(Widget::Anchor::TopLeft);
		s_canvas.fillRoundRect(p.x - border, p.y - border, 1000, s_resistanceWidget.getHeight() + border*2, border, k_resistanceColor);
		s_resistanceWidget.setTextColor(0x0);
		setUnitValue(s_targetWidget, s_targetResistance, 3, 999.999f, 1, 999.999f, "{");
		trackedColor = k_resistanceColor;
		trackedBorderY = p.y;
	}
	else
	{
		s_resistanceWidget.setTextColor(k_resistanceColor);
	}
	float resistance = s_current <= 0.0001f ? 999999999999.f : std::abs(s_voltage / s_current);
	setUnitValue(s_resistanceWidget, resistance, 3, 999.999f, 1, 999.999f, "{");
	s_resistanceWidget.render();

	float energy = getEnergy();
	setUnitValue(s_energyWidget, energy, 3, 999.999f, 3, 999.999f, "Wh");
	s_energyWidget.render();

	float charge = getCharge();
	setUnitValue(s_chargeWidget, charge, 3, 999.999f, 3, 999.999f, "Ah");
	s_chargeWidget.render();


	if (isLoadEnabled())
	{
		int16_t border = 3;
		Widget::Position p = s_timerWidget.getPosition(Widget::Anchor::TopLeft);
		s_canvas.fillRoundRect(p.x - border, p.y - border, s_timerWidget.getWidth() + border*2, s_timerWidget.getHeight() + border*2, border, k_timerColor);
		s_timerWidget.setTextColor(0x0);
	}
	else
	{
		s_timerWidget.setTextColor(k_timerColor);
	}
	{
		uint32_t tp = millis();
		uint32_t dt = tp > s_lastTimerTP ? tp - s_lastTimerTP : 0;
		s_lastTimerTP = tp;
		if (isLoadEnabled())
		{
			s_timer += dt;
		}
		int32_t seconds = (s_timer / 1000) % 60;
		int32_t minutes = (s_timer / 60000) % 60;
		int32_t hours = (s_timer / 3600000);
		char buf[128];
		sprintf(buf, "%d:%02d:%02d", hours, minutes, seconds);
		s_timerWidget.setValue(buf);
	}
	s_timerWidget.render();

	{
		int16_t border = 3;
		Widget::Position lp = s_targetLabelWidget.getPosition(Widget::Anchor::TopLeft);
		Widget::Position wp = s_targetWidget.getPosition(Widget::Anchor::BottomLeft);
		int16_t x = std::min(lp.x, wp.x);
		s_canvas.fillRoundRect(x - border, lp.y - border, 100, wp.y - lp.y + border*2, border, trackedColor);
		s_canvas.fillRect(s_canvas.width() - 10, trackedBorderY, 100, wp.y - trackedBorderY, trackedColor);
	}
  	s_targetLabelWidget.setPosition(s_targetLabelWidget.getPosition().alignX(s_targetWidget.getPosition()));
	s_targetLabelWidget.render();
	s_targetWidget.render();

	if (isLoadEnabled() && millis() >= s_loadEnabledTP + 1000) //skip a few samples
	{
		s_graphWidget.addValue(s_voltagePlot, s_timer, s_voltage, true);
		s_graphWidget.addValue(s_currentPlot, s_timer, s_current, true);
		s_graphWidget.addValue(s_powerPlot, s_timer, power, true);
		s_graphWidget.addValue(s_resistancePlot, s_timer, resistance, true);
		//s_graphWidget.addValue(s_energyPlot, s_timer, energy, true);
		//s_graphWidget.addValue(s_chargePlot, s_timer, charge, true);
	}
	s_graphWidget.render();

	if (s_menuSection != MenuSection::Disabled)
	{
		s_menu.render(s_canvas, 0);
	}

	static char program[1024] = { 0 };
	static size_t programSize = 0;
	if (Serial.available() > 0) 
	{
		char ch = Serial.peek();
		if (ch == 'r')
		{
			ESP.restart();
		}
		else while (Serial.available() > 0)
		{
			ch = Serial.read();
			if (ch == '\n' || ch == '\r' || programSize >= 1023)
			{
				program[programSize] = 0;
				compileProgram(program);
				programSize = 0;
				break;
			}
			else
			{
				program[programSize++] = ch;
			}
		}
	}
/*
	static int fanSpeed = 0;
	fanSpeed += s_knob.encoderChangedAcc();
	fanSpeed = std::min(std::max(fanSpeed, 0), 65535);
	printf("\nDAC: %d, temp: %f", fanSpeed, s_temperature);
	//setFanPWM(fanSpeed);
	setDAC(fanSpeed / 65535.f);
*/
	updateTracking();
	updateProgram();

	if (isLoadEnabled() || isRunningProgram())
	{
		//printOutput();
	}
}

void initMeasurementState()
{
	int16_t xSpacing = 12;
	int16_t ySpacing = 6;

	int16_t column1X = s_canvas.width() / 4;
	int16_t column2X = s_canvas.width() * 3 / 4;

	//1st ROW
	s_voltageWidget.setUseContentHeight(true);
	s_voltageWidget.setLimits(0, 99);
	s_voltageWidget.setTextColor(k_voltageColor);
	s_voltageWidget.setValueFont(&SansSerif_bold_32);
	s_voltageWidget.setSuffixFont(&SansSerif_bold_10);
  	s_voltageWidget.setDecimals(3);
  	s_voltageWidget.setPosition(Widget::Position{column1X, s_windowY + 8}, Widget::Anchor::TopCenter);

	s_currentWidget.setUseContentHeight(true);
	s_currentWidget.setLimits(0, 99);
	s_currentWidget.setTextColor(k_currentColor);
	s_currentWidget.setValueFont(&SansSerif_bold_32);
	s_currentWidget.setSuffixFont(&SansSerif_bold_10);
  	s_currentWidget.setDecimals(3);
  	s_currentWidget.setPosition(Widget::Position{column2X, s_windowY + 8}, Widget::Anchor::TopCenter);

	//2nd ROW
	s_energyWidget.setUseContentHeight(true);
	s_energyWidget.setLimits(0, 999.9999f);
	s_energyWidget.setTextColor(k_energyColor);
	s_energyWidget.setValueFont(&SansSerif_bold_18);
	s_energyWidget.setSuffixFont(&SansSerif_bold_10);
  	s_energyWidget.setDecimals(3);
  	s_energyWidget.setPosition(s_voltageWidget.getPosition(Widget::Anchor::BottomCenter).move(0, ySpacing), Widget::Anchor::TopCenter);

	s_powerWidget.setUseContentHeight(true);
	s_powerWidget.setLimits(0, 999.9999f);
	s_powerWidget.setTextColor(k_powerColor);
	s_powerWidget.setValueFont(&SansSerif_bold_18);
	s_powerWidget.setSuffixFont(&SansSerif_bold_10);
  	s_powerWidget.setDecimals(3);
  	s_powerWidget.setPosition(s_currentWidget.getPosition(Widget::Anchor::BottomCenter).move(0, ySpacing), Widget::Anchor::TopCenter);

	//3rd ROW
	s_chargeWidget.setUseContentHeight(true);
	s_chargeWidget.setLimits(0, 999.9999f);
	s_chargeWidget.setTextColor(k_chargeColor);
	s_chargeWidget.setValueFont(&SansSerif_bold_18);
	s_chargeWidget.setSuffixFont(&SansSerif_bold_10);
  	s_chargeWidget.setDecimals(3);
  	s_chargeWidget.setPosition(s_energyWidget.getPosition(Widget::Anchor::BottomCenter).move(0, ySpacing), Widget::Anchor::TopCenter);

	s_resistanceWidget.setUseContentHeight(true);
	s_resistanceWidget.setLimits(0, 999.9999f);
	s_resistanceWidget.setTextColor(k_resistanceColor);
	s_resistanceWidget.setValueFont(&SansSerif_bold_18);
	s_resistanceWidget.setSuffixFont(&SansSerif_bold_10);
  	s_resistanceWidget.setDecimals(3);
  	s_resistanceWidget.setPosition(s_powerWidget.getPosition(Widget::Anchor::BottomCenter).move(0, ySpacing), Widget::Anchor::TopCenter);

  	//4th ROW
	s_timerWidget.setUseContentHeight(true);
	s_timerWidget.setTextColor(k_timerColor);
	s_timerWidget.setFont(&SansSerif_bold_28);
  	s_timerWidget.setPosition(Widget::Position{xSpacing, s_chargeWidget.getPosition(Widget::Anchor::BottomLeft).y}.move(0, ySpacing * 2), Widget::Anchor::TopLeft);

	s_targetLabelWidget.setUseContentHeight(true);
	s_targetLabelWidget.setTextColor(0);
	s_targetLabelWidget.setFont(&SansSerif_bold_10);
  	s_targetLabelWidget.setPosition(Widget::Position{s_canvas.width() - xSpacing, s_chargeWidget.getPosition(Widget::Anchor::BottomLeft).y}.move(0, ySpacing * 2), Widget::Anchor::TopRight);

	s_targetWidget.setUseContentHeight(true);
	s_targetWidget.setLimits(0, 999.9999f);
	s_targetWidget.setTextColor(0);
	s_targetWidget.setValueFont(&SansSerif_bold_18);
	s_targetWidget.setSuffixFont(&SansSerif_bold_10);
  	s_targetWidget.setDecimals(3);
  	s_targetWidget.setPosition(Widget::Position{s_canvas.width() - xSpacing, s_targetLabelWidget.getPosition(Widget::Anchor::BottomLeft).y}.move(0, ySpacing), Widget::Anchor::TopRight);


	s_modeWidget.setFont(&SansSerif_bold_13);
	s_modeWidget.setPosition(Widget::Position{0, s_windowY - 3});

	s_graphWidget.setPosition(Widget::Position{0, s_timerWidget.getPosition(Widget::Anchor::BottomLeft).y}.move(0, ySpacing * 3), Widget::Anchor::TopLeft);
	s_graphWidget.setSize(s_canvas.width(), s_canvas.height() - s_graphWidget.getPosition().y - 1);
	s_voltagePlot = s_graphWidget.addPlot("V", k_voltageColor, 0.01f);
	s_currentPlot = s_graphWidget.addPlot("A", k_currentColor, 0.01f);
	s_powerPlot = s_graphWidget.addPlot("W", k_powerColor, 0.01f);
	s_resistancePlot = s_graphWidget.addPlot("{", k_resistanceColor, 0.01f);
	s_energyPlot = s_graphWidget.addPlot("Wh", k_energyColor, 0.00000001f);
	s_chargePlot = s_graphWidget.addPlot("Ah", k_chargeColor, 0.00000001f);

	//s_rangeWidget.setFont(&SansSerif_bold_13);
}

void beginMeasurementState()
{
	setLoadEnabled(false);
	setCurrentAutoRanging(true);
	setVoltageAutoRanging(true);

	s_menu.pushSubMenu({
	                 /* 0 */"Back",
					 /* 1 */"Target",
	                 /* 2 */"4 Wire: On",
					 /* 3 */"Mode: CC",
					 /* 4 */"Reset Energy",
					 /* 5 */"Start Program",
	                 /* 6 */"Stop Program",
					 /* 7 */"Settings",
	                }, 0, s_timerWidget.getPosition(Widget::Anchor::BottomLeft).y + 4);
}
void endMeasurementState()
{

}
