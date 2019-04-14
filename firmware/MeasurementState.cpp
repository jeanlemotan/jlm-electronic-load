#include "MeasurementState.h"
#include "Measurement.h"
#include "ValueWidget.h"
#include "LabelWidget.h"
#include "GraphWidget.h"
#include "Settings.h"
#include "Adafruit_GFX.h"
#include "AiEsp32RotaryEncoder.h"
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
extern Measurement s_measurement;

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
//static size_t s_menuSelection = 0;

void printOutput()
{
	static float voltageAcc = 0;
	static float currentAcc = 0;
	static size_t sampleCount = 0;

	voltageAcc += s_measurement.getVoltage();
	currentAcc += s_measurement.getCurrent();
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
	Serial.print(s_measurement.getEnergy(), 5);
	Serial.print(", ");
	Serial.print(s_measurement.getCharge(), 5);
	Serial.print(", ");
	Serial.print(s_measurement.getTemperature());
	Serial.println("");

	sampleCount = 0;
	voltageAcc = 0;
	currentAcc = 0;

	sampleIndex++;
}

static int32_t s_targetCurrent_mA = 0;
static int32_t s_targetPower_mW = 0;
static int32_t s_targetResistance_mO = 0;

static void refreshSubMenu()
{
	char buf[32];

	Measurement::TrackingMode mode = s_measurement.getTrackingMode();

	if (s_menuSection == MenuSection::Main)
	{
		sprintf(buf, "Target: %.3f %s", 
			(mode == Measurement::TrackingMode::CC ? s_targetCurrent_mA : mode == Measurement::TrackingMode::CP ? s_targetPower_mW : s_targetResistance_mO) / 1000.f,
			(mode == Measurement::TrackingMode::CC ? "A" : mode == Measurement::TrackingMode::CP ? "W" : "{"));
		s_menu.getSubMenuEntry(1).text = buf;
	}
	else if (s_menuSection == MenuSection::SetTarget)
	{
		sprintf(buf, "Target:>%.3f %s", 
			(mode == Measurement::TrackingMode::CC ? s_targetCurrent_mA : mode == Measurement::TrackingMode::CP ? s_targetPower_mW : s_targetResistance_mO) / 1000.f,
			(mode == Measurement::TrackingMode::CC ? "A" : mode == Measurement::TrackingMode::CP ? "W" : "{"));
		s_menu.getSubMenuEntry(1).text = buf;
	}
	sprintf(buf, "4 Wire: %s", s_measurement.is4WireEnabled() ? "On" : "Off");
	s_menu.getSubMenuEntry(2) = buf;

	if (mode == Measurement::TrackingMode::None)
	{
		sprintf(buf, "Mode: None");
	}
	else
	{
		sprintf(buf, "Mode: %s", mode == Measurement::TrackingMode::CC ? "CC" : mode == Measurement::TrackingMode::CP ? "CP" : "CR");
	}
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
	s_measurement.process();
	Measurement::TrackingMode mode = s_measurement.getTrackingMode();

	if (s_button.state() == Button::State::RELEASED)
	{
		s_measurement.setLoadEnabled(!s_measurement.isLoadEnabled());
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
			s_targetCurrent_mA = s_measurement.getTargetCurrent() * 1000.f;
			s_targetPower_mW = s_measurement.getTargetPower() * 1000.f;
			s_targetResistance_mO = s_measurement.getTargetResistance() * 1000.f;

			s_menuSection = MenuSection::SetTarget;
		}
		else if (selection == 2)
		{
			s_measurement.set4WireEnabled(!s_measurement.is4WireEnabled());
		}
		else if (selection == 3)
		{
			mode = mode == Measurement::TrackingMode::CC ? 
								Measurement::TrackingMode::CP : 
								mode == Measurement::TrackingMode::CP ? 
									Measurement::TrackingMode::CR : 
									Measurement::TrackingMode::CC;
			s_measurement.setTrackingMode(mode);
		}
		else if (selection == 4)
		{
			s_measurement.resetEnergy();
			s_graphWidget.clear();
		}
		else if (selection == 6)
		{
			stopProgram();
		}
	}
	else if (s_menuSection == MenuSection::SetTarget)
	{
		int knobDelta = s_knob.encoderChangedAcc();
		if (mode == Measurement::TrackingMode::CC)
		{
			s_targetCurrent_mA += knobDelta;
			s_targetCurrent_mA = std::max(std::min(s_targetCurrent_mA, int32_t(Measurement::k_maxCurrent * 1000.f)), 0);
		}
		else if (mode == Measurement::TrackingMode::CP)
		{
			s_targetPower_mW += knobDelta;
			s_targetPower_mW = std::max(std::min(s_targetPower_mW, int32_t(Measurement::k_maxPower * 1000.f)), 0);
		}
		else if (mode == Measurement::TrackingMode::CR)
		{
			s_targetResistance_mO += knobDelta;
			s_targetResistance_mO = std::max(std::min(s_targetResistance_mO, int32_t(Measurement::k_maxResistance * 1000.f)), int32_t(Measurement::k_minResistance * 1000.f));
		}
		refreshSubMenu();

		if (s_knob.currentButtonState() == BUT_RELEASED)
		{
			if (mode == Measurement::TrackingMode::CC)
			{
				s_measurement.setTargetCurrent(s_targetCurrent_mA / 1000.f);
			}
			else if (mode == Measurement::TrackingMode::CP)
			{
				s_measurement.setTargetPower(s_targetPower_mW / 1000.f);
			}
			else if (mode == Measurement::TrackingMode::CR)
			{
				s_measurement.setTargetResistance(s_targetResistance_mO / 1000.f);
			}
			s_menuSection = MenuSection::Main;
		}
	}
	else if (s_menuSection == MenuSection::Disabled)
	{
		if (s_knob.currentButtonState() == BUT_RELEASED)
		{
			s_menuSection = MenuSection::Main;
		}
	}


	//Mode
	s_modeWidget.setValue(isRunningProgram() ? "Program Mode" : mode == Measurement::TrackingMode::CC ? "CC Mode" : mode == Measurement::TrackingMode::CP ? "CP Mode" : "CR Mode");
	s_modeWidget.render();

	//char buf[32];
	//sprintf(buf, "(V%d/A%d)", getVoltageRange(), getCurrentRange());
	//s_rangeWidget.setValue(buf);
  	//s_rangeWidget.setPosition(Widget::Position{s_canvas.width() - s_rangeWidget.getWidth() - 2, s_windowY + s_rangeWidget.getHeight()});
	//s_rangeWidget.render();

	//s_voltageWidget.setValueColor(isVoltageValid() ? 0xFFFF : 0xF000);
	//s_voltageWidget.setDecimals(s_voltage < 10.f ? 3 : 2);
	float voltage = s_measurement.getVoltage();
	setUnitValue(s_voltageWidget, voltage, 3, 99.f, 1, 999.999f, "V");
	s_voltageWidget.render();

	uint16_t trackedColor = 0;
	int16_t trackedBorderY = 0;

	if (mode == Measurement::TrackingMode::CC)
	{
		int16_t border = 3;
		Widget::Position p = s_currentWidget.getPosition(Widget::Anchor::TopLeft);
		s_canvas.fillRoundRect(p.x - border, p.y - border, 1000, s_currentWidget.getHeight() + border*2, border, k_currentColor);
		//s_canvas.fillRect(p.x, p.y, s_currentWidget.getWidth(), s_currentWidget.getHeight(), k_currentColor),
		s_currentWidget.setTextColor(0x0);
		setUnitValue(s_targetWidget, s_measurement.getTargetCurrent(), 3, 99.f, 1, 999.999f, "A");
		trackedColor = k_currentColor;
		trackedBorderY = p.y;
	}
	else
	{
		s_currentWidget.setTextColor(k_currentColor);
	} 	
	float current = s_measurement.getCurrent();
	setUnitValue(s_currentWidget, current, 3, 99.f, 1, 999.999f, "A");
	s_currentWidget.render();

	if (mode == Measurement::TrackingMode::CP)
	{
		int16_t border = 3;
		Widget::Position p = s_powerWidget.getPosition(Widget::Anchor::TopLeft);
		s_canvas.fillRoundRect(p.x - border, p.y - border, 1000, s_powerWidget.getHeight() + border*2, border, k_powerColor);
		s_powerWidget.setTextColor(0x0);
		setUnitValue(s_targetWidget, s_measurement.getTargetPower(), 3, 999.999f, 1, 999.999f, "W");
		trackedColor = k_powerColor;
		trackedBorderY = p.y;
	}
	else
	{
		s_powerWidget.setTextColor(k_powerColor);
	}
	float power = s_measurement.getPower();
	setUnitValue(s_powerWidget, power, 3, 999.999f, 1, 999.999f, "W");
	s_powerWidget.render();

	if (mode == Measurement::TrackingMode::CR)
	{
		int16_t border = 3;
		Widget::Position p = s_resistanceWidget.getPosition(Widget::Anchor::TopLeft);
		s_canvas.fillRoundRect(p.x - border, p.y - border, 1000, s_resistanceWidget.getHeight() + border*2, border, k_resistanceColor);
		s_resistanceWidget.setTextColor(0x0);
		setUnitValue(s_targetWidget, s_measurement.getTargetResistance(), 3, 999.999f, 1, 999.999f, "{");
		trackedColor = k_resistanceColor;
		trackedBorderY = p.y;
	}
	else
	{
		s_resistanceWidget.setTextColor(k_resistanceColor);
	}
	float resistance = s_measurement.getResistance();
	setUnitValue(s_resistanceWidget, resistance, 3, 999.999f, 1, 999.999f, "{");
	s_resistanceWidget.render();

	float energy = s_measurement.getEnergy();
	setUnitValue(s_energyWidget, energy, 3, 999.999f, 3, 999.999f, "Wh");
	s_energyWidget.render();

	float charge = s_measurement.getCharge();
	setUnitValue(s_chargeWidget, charge, 3, 999.999f, 3, 999.999f, "Ah");
	s_chargeWidget.render();

	Clock::duration loadTimer = s_measurement.getLoadTimer();

	if (s_measurement.isLoadEnabled())
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
		int32_t seconds = std::chrono::duration_cast<std::chrono::seconds>(loadTimer).count() % 60;
		int32_t minutes = std::chrono::duration_cast<std::chrono::minutes>(loadTimer).count() % 60;
		int32_t hours = std::chrono::duration_cast<std::chrono::hours>(loadTimer).count();
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

	if (s_measurement.isLoadEnabled() && s_measurement.isLoadSettled()) //skip a few samples
	{
		s_graphWidget.addValue(s_voltagePlot, loadTimer, voltage, true);
		s_graphWidget.addValue(s_currentPlot, loadTimer, current, true);
		s_graphWidget.addValue(s_powerPlot, loadTimer, power, true);
		s_graphWidget.addValue(s_resistancePlot, loadTimer, resistance, true);
		//s_graphWidget.addValue(s_energyPlot, loadTimer, energy, true);
		//s_graphWidget.addValue(s_chargePlot, loadTimer, charge, true);
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
	updateProgram();

	if (s_measurement.isLoadEnabled() || isRunningProgram())
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
	s_measurement.setLoadEnabled(false);
	s_measurement.setCurrentAutoRanging(true);
	s_measurement.setVoltageAutoRanging(true);

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
