#include "CalibrationState.h"
#include "MeasurementState.h"
#include "ValueWidget.h"
#include "LabelWidget.h"
#include "Settings.h"
#include "ADS1115.h"
#include "Adafruit_SSD1351.h"
#include "AiEsp32RotaryEncoder.h"
#include "Menu.h"
#include "State.h"

extern Adafruit_SSD1351 s_display;
extern AiEsp32RotaryEncoder s_knob;
extern ADS1115 s_adc;
extern Settings s_settings;

extern LabelWidget s_modeWidget;
static ValueWidget s_temperatureWidget(s_display, 0.f, "'C");
static ValueWidget s_voltageWidget(s_display, 0.f, "V");
static ValueWidget s_currentWidget(s_display, 0.f, "A");
static ValueWidget s_powerWidget(s_display, 0.f, "W");
static ValueWidget s_trackedWidget(s_display, 0.f, "A");
static ValueWidget s_dacWidget(s_display, 0.f, "");

enum class CalibrationItem
{
	Voltage,
	Current,
	Temperature
};
static CalibrationItem s_calibrationItem = CalibrationItem::Voltage;

static Menu s_menu;
static size_t s_section = 0;
static size_t s_range = 4;
static float s_value = 0.f;
static size_t s_sampleCount = 0;
static float s_v1Vref = 5.0f;
static float s_v1Value = 1.313f;
static float s_v2Vref = 30.0f;
static float s_v2Value = 3.0f;
static Settings s_savedSettings;

void processCalibrationState()
{
	bool readVoltage, readCurrent, readTemperature;
	readAdcs(readVoltage, readCurrent, readTemperature);

	readAdcs();

	//Mode
	s_modeWidget.setValue("Calibration");
	s_modeWidget.update();

	if (s_section == 0)
	{
		size_t selection = s_menu.process(s_knob);

		if (selection == 0)
		{
			setState(State::Measurement);
			return;
		}
		if (selection != size_t(-1))
		{
			s_section = selection;
			s_savedSettings = s_settings;
			if (selection == 4 || selection == 5)
			{
				s_value = 0.f;
				s_sampleCount = 0;
			}
		}
	}
	else if (s_section == 1) //range
	{
		int knobDelta = s_knob.encoderChangedAcc();
		s_range += knobDelta;
		s_range = std::min<uint32_t>(s_range, Settings::k_rangeCount);
		char buf[32];
		sprintf(buf, "Range: %d", s_range);
		s_menu.setSubMenuEntry(1, buf);
		if (s_knob.currentButtonState() == BUT_RELEASED)
		{
			setVoltageRange(s_range);
			s_section = 0;
		}
	}
	else if (s_section == 2) //v1 vref
	{
		int knobDelta = s_knob.encoderChangedAcc();
		s_v1Vref += knobDelta / 1000.f;
		s_v1Vref = std::max(s_v1Vref, 0.f);
		char buf[32];
		sprintf(buf, "V1 Vref: %.3f", s_v1Vref);
		s_menu.setSubMenuEntry(2, buf);
		if (s_knob.currentButtonState() == BUT_RELEASED)
		{
			s_section = 0;
		}
	}
	else if (s_section == 3) //v2 vref
	{
		int knobDelta = s_knob.encoderChangedAcc();
		s_v2Vref += knobDelta / 1000.f;
		s_v2Vref = std::max(s_v2Vref, 0.f);
		char buf[32];
		sprintf(buf, "V2 Vref: %.3f", s_v2Vref);
		s_menu.setSubMenuEntry(3, buf);
		if (s_knob.currentButtonState() == BUT_RELEASED)
		{
			s_section = 0;
		}
	}
	else if (s_section == 4) //V1
	{
		s_menu.process(s_knob);
		if (!isSwitchingVoltageRange() && readVoltage)
		{
			s_value += getVoltageRaw();
			s_sampleCount++;
			char buf[32];
			sprintf(buf, "*** %d: %.5f", s_sampleCount, s_value / (float)s_sampleCount);
			s_menu.setSubMenuEntry(4, buf);
		}
		if (s_sampleCount > 20)
		{
			s_v1Value = (s_value / (float)s_sampleCount);
			ESP_LOGI("Calibration", "V1 value for range %d: %f", s_range, s_v1Value);
			s_value = 0.f;
			s_sampleCount = 0;

			s_menu.setSubMenuEntry(4, "V1 Calibration");
			s_section = 0;
		}
	}
	else if (s_section == 5) //V2
	{
		s_menu.process(s_knob);
		if (!isSwitchingVoltageRange() && readVoltage)
		{
			s_value += getVoltageRaw();
			s_sampleCount++;
			char buf[32];
			sprintf(buf, "*** %d: %.5f", s_sampleCount, s_value / (float)s_sampleCount);
			s_menu.setSubMenuEntry(5, buf);
		}
		if (s_sampleCount > 20)
		{
			s_v2Value = (s_value / (float)s_sampleCount);
			ESP_LOGI("Calibration", "V2 value for range %d: %f", s_range, s_v2Value);
			s_value = 0.f;
			s_sampleCount = 0;

			float scale = (s_v2Value - s_v1Value) / (s_v2Vref - s_v1Vref);
			float bias = (s_v1Value - scale * s_v1Vref);
			ESP_LOGI("Calibration", "Range %d bias: %f, scale: %f", s_range, bias, scale);

			s_settings.voltageRangeBiases[s_range] = -bias;
			s_settings.voltageRangeScales[s_range] = 1.f / scale;
			saveSettings(s_settings);
			loadSettings(s_settings);

			s_menu.setSubMenuEntry(5, "V2 Calibration");
			s_section = 0;
		}
	}
	s_menu.render(s_display, 0);
}

void initCalibrationState()
{
}

void beginCalibrationState()
{
	s_section = 0;
	s_menu.pushSubMenu({
	                 "<-",
	                 "Range",
	                 "V1 Vref: 0.0000V",
	                 "V2 Vref: 0.0000V",
	                 "V1 Calibration",
	                 "V2 Calibration",
	                }, 0, 12);

	char buf[32];
	sprintf(buf, "Range: %d", s_range);
	s_menu.setSubMenuEntry(1, buf);
	sprintf(buf, "V1 Vref: %.3f", s_v1Vref);
	s_menu.setSubMenuEntry(2, buf);
	sprintf(buf, "V2 Vref: %.3f", s_v2Vref);
	s_menu.setSubMenuEntry(3, buf);
}

void endCalibrationState()
{
	s_menu.popSubMenu();
}
