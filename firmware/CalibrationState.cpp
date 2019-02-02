#include "CalibrationState.h"
#include "MeasurementState.h"
#include "ValueWidget.h"
#include "LabelWidget.h"
#include "Settings.h"
#include "ADS1115.h"
#include "Adafruit_GFX.h"
#include "AiEsp32RotaryEncoder.h"
#include "Menu.h"
#include "State.h"
#include "PWM.h"

extern GFXcanvas16 s_canvas;
extern AiEsp32RotaryEncoder s_knob;
extern ADS1115 s_adc;
extern Settings s_settings;

extern LabelWidget s_modeWidget;

enum class CalibrationItem
{
	Voltage,
	Current,
	Temperature
};
static CalibrationItem s_calibrationItem = CalibrationItem::Voltage;

static Menu s_menu;
static size_t s_section = 0;

static size_t s_type = 0; //0 - V, 1 - A, 2 - 'C
static size_t s_range = 4;
static float s_raw = 0.f;
static float s_total = 0.f;
static size_t s_sampleCount = 0;
static size_t s_sampleSkipCount = 0;

static float s_ref1 = 0.1f;
static float s_ref2 = 10.0f;

static std::array<float, Settings::k_rangeCount> s_savedRef1;
static std::array<float, Settings::k_rangeCount> s_savedValue1;
static std::array<float, Settings::k_rangeCount> s_savedRef2;
static std::array<float, Settings::k_rangeCount> s_savedValue2;

static Settings s_newSettings;

const char* getUnit()
{
	switch (s_type)
	{
		case 0: return "V";
		case 1: return "A";
		case 2: return "'C";
	}
	return "N/A";
}

void readData()
{
	bool readVoltage, readCurrent, readTemperature;
	readAdcs(readVoltage, readCurrent, readTemperature);

	bool read = false;
	if (s_type == 0 && !isSwitchingVoltageRange() && readVoltage)
	{
		read = true;
		s_raw = getVoltageRaw();
	}
	else if (s_type == 1 && !isSwitchingCurrentRange() && readCurrent)
	{
		read = true;
		s_raw = getCurrentRaw();
	}
	else if (s_type == 2 && readTemperature)
	{
		read = true;
		s_raw = getTemperatureRaw();
	}

	if (read)
	{
		s_sampleSkipCount++;
		if (s_sampleSkipCount > 10)
		{
			s_total += s_raw;
			s_sampleCount++;
		}
	}
}

void refreshMenu()
{
	char buf[32];

	sprintf(buf, "Sensor: %.5f %s", s_raw, getUnit());
	s_menu.setSubMenuEntry(1, buf);

	sprintf(buf, "Range: %d %s", s_range, s_type == 2 ? "(locked)" : "");
	s_menu.setSubMenuEntry(2, buf);
	
	sprintf(buf, "Ref1: %.4f %s", s_ref1, getUnit());
	s_menu.setSubMenuEntry(3, buf);
	sprintf(buf, "Ref2: %.4f %s", s_ref2, getUnit());
	s_menu.setSubMenuEntry(4, buf);

	sprintf(buf, "Calibrate 1st");
	s_menu.setSubMenuEntry(5, buf);
	sprintf(buf, "Calibrate 2nd");
	s_menu.setSubMenuEntry(6, buf);
}

void processCalibrationState()
{
	char buf[64];

	//Mode
	s_modeWidget.setValue("Calibration");
	s_modeWidget.update();

	if (s_section == 0)
	{
		readData();
		refreshMenu();

		size_t selection = s_menu.process(s_knob);

		if (selection == 0)
		{
			setState(State::Measurement);
			return;
		}
		if (selection != size_t(-1))
		{
			s_section = selection;
			if (selection == 5 || 
				selection == 6)
			{
				s_total = 0.f;
				s_sampleCount = 0;
				s_sampleSkipCount = 0;
				if (s_type == 1) //enable the load
				{
					setDACPWM(1.f);
				}
			}
		}
	}
	else if (s_section == 1) //type
	{
		int knobDelta = s_knob.encoderChanged();
		s_type += knobDelta;
		s_type %= 3;
		readData();
		refreshMenu();
		if (s_knob.currentButtonState() == BUT_RELEASED)
		{
			s_section = 0;
		}
		s_range = 0;
	}
	else if (s_section == 2) //range
	{
		int knobDelta = s_knob.encoderChanged();
		if (s_type == 2)
		{
			s_range = 0;
		}
		else
		{
			s_range += knobDelta;
			s_range %= Settings::k_rangeCount;
		}
		if (s_knob.currentButtonState() == BUT_RELEASED)
		{
			setVoltageRange(s_range);
			setCurrentRange(s_range);
			s_section = 0;
		}
		readData();
		refreshMenu();
	}
	else if (s_section == 3) //ref1
	{
		int knobDelta = s_knob.encoderChangedAcc();
		s_ref1 += knobDelta / 10000.f;
		s_ref1 = std::max(s_ref1, 0.f);
		readData();
		refreshMenu();
		if (s_knob.currentButtonState() == BUT_RELEASED)
		{
			s_section = 0;
		}
	}
	else if (s_section == 4) //ref2
	{
		int knobDelta = s_knob.encoderChangedAcc();
		s_ref2 += knobDelta / 10000.f;
		s_ref2 = std::max(s_ref2, 0.f);
		readData();
		refreshMenu();
		if (s_knob.currentButtonState() == BUT_RELEASED)
		{
			s_section = 0;
		}
	}
	else if (s_section == 5) //Calibration 1
	{
		s_menu.process(s_knob);
		readData();
		if (s_sampleCount > 0)
		{
			sprintf(buf, "*** %d: %.5f", s_sampleCount, s_total / (float)s_sampleCount);
			s_menu.setSubMenuEntry(5, buf);
		}
		else
		{
			sprintf(buf, "Waiting...");
			s_menu.setSubMenuEntry(5, buf);
		}

		if (s_sampleCount > 20)
		{
			s_savedRef1[s_range] = s_ref1;
			s_savedValue1[s_range] = (s_total / (float)s_sampleCount);
			ESP_LOGI("Calibration", "Value1 for range %d: %f", s_range, s_savedValue1[s_range]);
			s_total = 0.f;
			s_sampleCount = 0;

			refreshMenu();
			s_section = 0;
			setDACPWM(0.f);
		}
	}
	else if (s_section == 6) //Calibration 2
	{
		s_menu.process(s_knob);
		readData();
		if (s_sampleCount > 0)
		{
			sprintf(buf, "*** %d: %.5f", s_sampleCount, s_total / (float)s_sampleCount);
			s_menu.setSubMenuEntry(6, buf);
		}
		else
		{
			sprintf(buf, "Waiting...");
			s_menu.setSubMenuEntry(6, buf);
		}
		if (s_sampleCount > 20)
		{
			s_savedRef2[s_range] = s_ref2;
			s_savedValue2[s_range] = (s_total / (float)s_sampleCount);
			ESP_LOGI("Calibration", "Value2 for range %d: %f", s_range, s_savedValue2[s_range]);
			s_total = 0.f;
			s_sampleCount = 0;

			float scale = (s_savedValue2[s_range] - s_savedValue1[s_range]) / (s_savedRef2[s_range] - s_savedRef1[s_range]);
			float bias = (s_savedValue1[s_range] - scale * s_savedRef1[s_range]);
			ESP_LOGI("Calibration", "Range %d bias: %f, scale: %f", s_range, bias, scale);

			if (s_type == 0)
			{
				s_newSettings.voltageRangeBiases[s_range] = -bias;
				s_newSettings.voltageRangeScales[s_range] = 1.f / scale;
			}
			else if (s_type == 1)
			{
				s_newSettings.currentRangeBiases[s_range] = -bias;
				s_newSettings.currentRangeScales[s_range] = 1.f / scale;
			}
			else if (s_type == 2)
			{
				s_newSettings.temperatureBias = -bias;
				s_newSettings.temperatureScale = 1.f / scale;
			}

			refreshMenu();
			s_section = 0;
			setDACPWM(0.f);
		}
	}
	else if (s_section == 7) //save
	{
		saveSettings(s_newSettings);
		loadSettings(s_newSettings);
		s_settings = s_newSettings;
		s_section = 0;
	}
	s_menu.render(s_canvas, 0);
}

void initCalibrationState()
{
}

void beginCalibrationState()
{
	s_newSettings = s_settings;

	s_type = 0;
	s_range = 4;
	s_total = 0.f;
	s_sampleCount = 0;
	s_sampleSkipCount = 0;

	s_ref1 = 0.1f;
	s_ref2 = 10.0f;

	s_savedRef1 = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
	s_savedValue1 = { 0.f, 0.f, 0.f, 0.f, 0.f };
	s_savedRef2 = { 10.0f, 10.0f, 10.0f, 10.0f, 10.0f };
	s_savedValue2 = { 0.f, 0.f, 0.f, 0.f, 0.f };	

	s_section = 0;
	s_menu.pushSubMenu({
	                 /* 0 */"<-",
					 /* 1 */"Type: V",
	                 /* 2 */"Range",
	                 /* 3 */"Ref1: 0.0000V",
	                 /* 4 */"Ref2: 0.0000V",
	                 /* 5 */"Calibration1",
	                 /* 6 */"Calibration2",
					 /* 7 */"Save",
	                }, 0, 12);

	refreshMenu();
}

void endCalibrationState()
{
	s_menu.popSubMenu();
}
