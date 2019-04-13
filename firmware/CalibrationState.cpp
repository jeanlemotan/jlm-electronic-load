#include "CalibrationState.h"
#include "Measurement.h"
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
extern int16_t s_windowY;
extern AiEsp32RotaryEncoder s_knob;
extern ADS1115 s_adc;
extern Settings s_settings;

extern LabelWidget s_modeWidget;

enum class MenuSection
{
	Main,
	Voltage,
	Current,
	Temperature,
	DAC
};
static MenuSection s_menuSection = MenuSection::Main;

static Menu s_menu;
static size_t s_selection = 0;

static size_t s_range = 4;
static float s_raw = 0.f;
static float s_total = 0.f;
static size_t s_sampleCount = 0;
static size_t s_sampleSkipCount = 0;

static size_t s_dacTableIndex = 0;

static float s_ref1 = 0.1f;
static float s_ref2 = 10.0f;

static std::array<float, Settings::k_rangeCount> s_savedRef1;
static std::array<float, Settings::k_rangeCount> s_savedValue1;
static std::array<float, Settings::k_rangeCount> s_savedRef2;
static std::array<float, Settings::k_rangeCount> s_savedValue2;

static Settings s_newSettings;

static const char* getUnit()
{
	switch (s_menuSection)
	{
		case MenuSection::Voltage: return "V";
		case MenuSection::Current: return "A";
		case MenuSection::Temperature: return "'C";
		default: return "";
	}
	return "";
}

static void readData()
{
	bool readVoltage, readCurrent, readTemperature;
	readAdcs(readVoltage, readCurrent, readTemperature);

	bool read = false;
	if (s_menuSection == MenuSection::Voltage && readVoltage)
	{
		read = true;
		s_raw = getVoltageRaw();
	}
	else if (s_menuSection == MenuSection::Current && readCurrent)
	{
		read = true;
		s_raw = getCurrentRaw();
	}
	else if (s_menuSection == MenuSection::Temperature && readTemperature)
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

static void refreshSubMenu()
{
	char buf[32];

	if (s_menuSection == MenuSection::DAC)
	{
		sprintf(buf, "DAC: %d%%", int(getDAC() * 100.f));
		s_menu.getSubMenuEntry(1) = buf;
		sprintf(buf, "Current: %f", getCurrent());
		s_menu.getSubMenuEntry(2) = buf;
	}
	else
	{
		sprintf(buf, "Raw: %.5f %s", s_raw, getUnit());
		s_menu.getSubMenuEntry(1) = buf;

		sprintf(buf, "Range: %d %s", s_range, s_menuSection == MenuSection::Temperature ? "(locked)" : "");
		s_menu.getSubMenuEntry(2) = buf;
		
		sprintf(buf, "Ref1: %.4f %s", s_ref1, getUnit());
		s_menu.getSubMenuEntry(3) = buf;
		sprintf(buf, "Ref2: %.4f %s", s_ref2, getUnit());
		s_menu.getSubMenuEntry(4) = buf;

		sprintf(buf, "Calibrate 1st");
		s_menu.getSubMenuEntry(5) = buf;
		sprintf(buf, "Calibrate 2nd");
		s_menu.getSubMenuEntry(6) = buf;
	}
}

static void processMainSection()
{
	char buf[64];

	//Mode
	s_modeWidget.setValue("Cal");
	s_modeWidget.render();

	size_t selection = s_menu.process(s_knob);
	if (selection == 0)
	{
		setState(State::Measurement);
		return;
	}
	if (selection != size_t(-1))
	{
		s_menuSection = MenuSection(selection);
		s_selection = 0;
		s_range = 0;
		setVoltageRange(s_range);
		setCurrentRange(s_range);	
		setDAC(0.f);
		setLoadEnabled(false);
		if (s_menuSection == MenuSection::DAC)
		{
			s_menu.pushSubMenu({
				                 /* 0 */"Back",
								 /* 1 */"Start",
								 /* 2 */"Save",
				                }, 0, s_windowY);
		}
		else
		{
			s_menu.pushSubMenu({
				                 /* 0 */"Back",
								 /* 1 */"Raw: V",
				                 /* 2 */"Range",
				                 /* 3 */"Ref1: 0.0000V",
				                 /* 4 */"Ref2: 0.0000V",
				                 /* 5 */"Calibration1",
				                 /* 6 */"Calibration2",
								 /* 7 */"Save",
				                }, 0, s_windowY);
		}
	}
	s_menu.render(s_canvas, 0);
}

static void processDACSection()
{
	//Mode
	s_modeWidget.setValue("Cal / DAC");
	s_modeWidget.render();

	if (s_selection == 0)
	{
		size_t selection = s_menu.process(s_knob);
		//ESP_LOGI("Calibration", "selection %d", selection);
		if (selection == 0)
		{
			s_menuSection = MenuSection::Main;
			s_selection = 0;
			s_menu.popSubMenu();
			return;
		}
		else if (selection == 1)
		{
			s_selection = 1;
			s_menu.pushSubMenu({
				                 /* 0 */"Back",
								 /* 1 */"DAC",
								 /* 2 */"Current",
				                }, 0, s_windowY);
			setCurrentAutoRanging(true);
			setDAC(0.f);
			setLoadEnabled(false);
			s_total = 0;
			s_sampleSkipCount = 0;
			s_sampleCount = 0;
			s_dacTableIndex = 0;
			//setFanPWM(0.75f);
		}
		else if (selection == 2)
		{
			saveSettings(s_newSettings);
			loadSettings(s_newSettings);
			s_settings = s_newSettings;
		}
	}
	else
	{
		refreshSubMenu();
		size_t selection = s_menu.process(s_knob);
		if (selection == 0)
		{
			setFanPWM(0);
			setDAC(0);
			setLoadEnabled(false);
			s_selection = 0;
			s_menu.popSubMenu();
			return;
		}

		bool readVoltage, readCurrent, readTemperature;
		readAdcs(readVoltage, readCurrent, readTemperature);
		if (readCurrent)
		{
			s_sampleSkipCount++;
			if (s_sampleSkipCount == 3)
			{
				ESP_LOGI("Calibration", "Load on");
				setLoadEnabled(true);
			}
			else if (s_sampleSkipCount >= 10)
			{
				float value = getCurrent();
				ESP_LOGI("Calibration", "Sample: %f", value);
				s_total += value;
				s_sampleCount++;
			}
			else
			{
				ESP_LOGI("Calibration", "Skipping, waiting to stabilize");
			}
		}
		if (s_sampleCount >= 10)
		{
			setLoadEnabled(false);
			float value = s_total / (float)s_sampleCount;
			s_newSettings.dac2CurrentTable[s_dacTableIndex] = value;
			ESP_LOGI("Calibration", "DAC index %d: %f", s_dacTableIndex, value);
			s_total = 0;
			s_sampleSkipCount = 0;
			s_sampleCount = 0;
			s_dacTableIndex++;
			if (s_dacTableIndex >= s_newSettings.dac2CurrentTable.size() || value >= k_maxCurrent)
			{
				setFanPWM(0);
				setDAC(0);
				setLoadEnabled(false);
				s_selection = 0;
				s_menu.popSubMenu();
				return;
			}
			else
			{
				float f = (float)s_dacTableIndex / (float)s_newSettings.dac2CurrentTable.size();
				setDAC(f * f * f);
			}
		}
	}
	s_menu.render(s_canvas, 0);
}

static void process2PointSection()
{
	char buf[64];

	//Mode
	sprintf(buf, "Cal / %s", getUnit());
	s_modeWidget.setValue(buf);
	s_modeWidget.render();

	if (s_selection == 0)
	{
		readData();
		refreshSubMenu();

		size_t selection = s_menu.process(s_knob);

		if (selection == 0)
		{
			s_menuSection = MenuSection::Main;
			s_selection = 0;
			s_menu.popSubMenu();
			return;
		}
		if (selection != size_t(-1) && selection != 1)
		{
			s_selection = selection;
			if (selection == 5 || 
				selection == 6)
			{
				s_total = 0.f;
				s_sampleCount = 0;
				s_sampleSkipCount = 0;
				if (s_menuSection == MenuSection::Current) //enable the load
				{
					setDAC(1.f);
					setLoadEnabled(true);
				}
			}
		}
	}
	else if (s_selection == 2) //range
	{
		int knobDelta = s_knob.encoderChanged();
		if (s_menuSection == MenuSection::Temperature)
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
			s_selection = 0;
		}
		readData();
		refreshSubMenu();
	}
	else if (s_selection == 3) //ref1
	{
		int knobDelta = s_knob.encoderChangedAcc();
		s_ref1 += knobDelta / 10000.f;
		s_ref1 = std::max(s_ref1, 0.f);
		readData();
		refreshSubMenu();
		if (s_knob.currentButtonState() == BUT_RELEASED)
		{
			s_selection = 0;
		}
	}
	else if (s_selection == 4) //ref2
	{
		int knobDelta = s_knob.encoderChangedAcc();
		s_ref2 += knobDelta / 10000.f;
		s_ref2 = std::max(s_ref2, 0.f);
		readData();
		refreshSubMenu();
		if (s_knob.currentButtonState() == BUT_RELEASED)
		{
			s_selection = 0;
		}
	}
	else if (s_selection == 5) //Calibration 1
	{
		s_menu.process(s_knob);
		readData();
		if (s_sampleCount > 0)
		{
			sprintf(buf, "*** %d: %.5f", s_sampleCount, s_total / (float)s_sampleCount);
			s_menu.getSubMenuEntry(5) = buf;
		}
		else
		{
			sprintf(buf, "Waiting...");
			s_menu.getSubMenuEntry(5) = buf;
		}

		if (s_sampleCount > 20)
		{
			s_savedRef1[s_range] = s_ref1;
			s_savedValue1[s_range] = (s_total / (float)s_sampleCount);
			ESP_LOGI("Calibration", "Value1 for range %d: %f", s_range, s_savedValue1[s_range]);
			s_total = 0.f;
			s_sampleCount = 0;

			refreshSubMenu();
			s_selection = 0;
			setDAC(0.f);
			setLoadEnabled(false);
		}
	}
	else if (s_selection == 6) //Calibration 2
	{
		s_menu.process(s_knob);
		readData();
		if (s_sampleCount > 0)
		{
			sprintf(buf, "*** %d: %.5f", s_sampleCount, s_total / (float)s_sampleCount);
			s_menu.getSubMenuEntry(6) = buf;
		}
		else
		{
			sprintf(buf, "Waiting...");
			s_menu.getSubMenuEntry(6) = buf;
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

			if (s_menuSection == MenuSection::Voltage)
			{
				s_newSettings.voltageRangeBiases[s_range] = -bias;
				s_newSettings.voltageRangeScales[s_range] = 1.f / scale;
			}
			else if (s_menuSection == MenuSection::Current)
			{
				s_newSettings.currentRangeBiases[s_range] = -bias;
				s_newSettings.currentRangeScales[s_range] = 1.f / scale;
			}
			else if (s_menuSection == MenuSection::Temperature)
			{
				s_newSettings.temperatureBias = -bias;
				s_newSettings.temperatureScale = 1.f / scale;
			}

			refreshSubMenu();
			s_selection = 0;
			setDAC(0.f);
			setLoadEnabled(false);
		}
	}
	else if (s_selection == 7) //save
	{
		saveSettings(s_newSettings);
		loadSettings(s_newSettings);
		s_settings = s_newSettings;
		s_selection = 0;
	}
	s_menu.render(s_canvas, 0);	
}

void processCalibrationState()
{
	if (s_menuSection == MenuSection::Main)
	{
		processMainSection();
	}
	else if (s_menuSection == MenuSection::DAC)
	{
		processDACSection();
	}
	else
	{
		process2PointSection();
	}
}

void initCalibrationState()
{
}

void beginCalibrationState()
{
	s_newSettings = s_settings;

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

	s_selection = 0;
	s_menuSection = MenuSection::Main;

	s_menu.pushSubMenu({
	                 /* 0 */"Back",
					 /* 1 */"Voltage",
	                 /* 2 */"Current",
	                 /* 3 */"Temperature",
	                 /* 4 */"DAC",
	                }, 0, s_windowY);

	setVoltageRange(s_range);
	setCurrentRange(s_range);	
	setDAC(0.f);
	setLoadEnabled(false);
}

void endCalibrationState()
{
	if (s_menuSection != MenuSection::Main)
	{
		s_menu.popSubMenu(); //sub
	}
	s_menu.popSubMenu(); //main
}
