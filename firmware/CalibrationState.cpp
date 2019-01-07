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
static size_t s_range = 0;
static float s_value = 0.f;
static size_t s_sampleCount = 0;
static Settings s_savedSettings;

void processCalibrationState()
{
	readAdcs();

	//Mode
	s_modeWidget.setValue("Calibration");
	s_modeWidget.update();

	size_t selection = s_menu.process(s_knob);

	if (s_section == 0)
	{
		if (selection == 0)
		{
			setState(State::Measurement);
			return;
		}
		if (selection != size_t(-1))
		{
			s_section = selection;
			s_savedSettings = s_settings;
			if (selection >= 1 && selection <= 4)
			{
				s_range = 0;
				s_value = 0.f;
				s_sampleCount = 0;
				if (selection >= 1 && selection <= 2)
				{
					setVoltageRange(s_range);
				}
				else
				{
					setCurrentRange(s_range);
				}
				s_menu.pushSubMenu({
				                 "Range 0: ...",
				                 " ",
				                 " ",
				                 " ",
				                 " ",
				                }, 0, 12);

			}
		}
	}
	else if (s_section == 1) //voltage bias
	{
		if (!isSwitchingVoltageRange())
		{
			s_value += getVoltageRaw();
			s_sampleCount++;
			char buf[32];
			sprintf(buf, "Range %d: %.5f", s_range, s_value / (float)s_sampleCount);
			s_menu.setSubMenuEntry(s_range, buf);
		}
		if (s_sampleCount > 100)
		{
			s_settings.voltageRangeBiases[s_range] = -(s_value / (float)s_sampleCount);
			s_value = 0.f;
			s_sampleCount = 0;
			s_range++;
			if (s_range < Settings::k_rangeCount)
			{
				setVoltageRange(s_range);
			}
			else
			{
				s_menu.popSubMenu();
				s_section = 0;
			}
		}
	}
	else if (s_section == 2) //voltage scale
	{
		if (!isSwitchingVoltageRange())
		{
			s_value += getVoltageRaw() + s_settings.voltageRangeBiases[s_range];
			s_sampleCount++;
			char buf[32];
			sprintf(buf, "Range %d: %.5f", s_range, s_value / (float)s_sampleCount);
			s_menu.setSubMenuEntry(s_range, buf);
		}
		if (s_sampleCount > 100)
		{
			s_settings.voltageRangeScales[s_range] = 1.3214f / (s_value / (float)s_sampleCount);
			s_value = 0.f;
			s_sampleCount = 0;
			s_range++;
			if (s_range < Settings::k_rangeCount)
			{
				setVoltageRange(s_range);
			}
			else
			{
				s_menu.popSubMenu();
				s_section = 0;
			}
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
	                 "Voltage Bias",
	                 "Voltage Scale",
	                 "Current Bias",
	                 "Current Scale",
	                 "Temperature Bias",
	                 "Temperature Scale",
	                }, 0, 12);
}

void endCalibrationState()
{
	s_menu.popSubMenu();
}
