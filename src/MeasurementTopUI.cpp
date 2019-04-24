#include "Measurement.h"
#include "MeasurementTopUI.h"
#include "ValueWidget.h"
#include "LabelWidget.h"
#include "DurationEditWidget.h"
#include "ValueEditWidget.h"
#include "GraphWidget.h"
#include "ImageWidget.h"
#include "Settings.h"
#include "DeltaBitmap.h"
#include "AiEsp32RotaryEncoder.h"
#include "Button.h"
#include <driver/adc.h>
#include "Program.h"
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
#include "icons.h"

extern DeltaBitmap s_canvas;
extern int16_t s_windowY;
extern AiEsp32RotaryEncoder s_knob;
extern Measurement s_measurement;
extern ValueWidget s_temperatureWidget;

extern LabelWidget s_modeWidget;
//static LabelWidget s_rangeWidget(s_canvas, "");
static ValueWidget s_voltageWidget(s_canvas, 0.f, "V");
static ValueWidget s_currentWidget(s_canvas, 0.f, "A");
static ValueWidget s_resistanceWidget(s_canvas, 0.f, "{"); //Create the ohm symbol with this: https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
static ValueWidget s_powerWidget(s_canvas, 0.f, "W");
static ValueWidget s_energyWidget(s_canvas, 0.f, "Wh");
static ValueWidget s_chargeWidget(s_canvas, 0.f, "Ah");
static LabelWidget s_timerWidget(s_canvas, "00:00:00");
//static DurationEditWidget s_timerWidget2(s_canvas, nullptr);
//static ValueEditWidget s_timerWidget2(s_canvas, nullptr);
static ValueWidget s_targetWidget(s_canvas, 0.f, "Ah");

static ImageWidget s_graphIconWidget(s_canvas, &k_imgGraph, nullptr, nullptr);
static ImageWidget s_limitsIconWidget(s_canvas, &k_imgLimits, nullptr, nullptr);
static ImageWidget s_settingsIconWidget(s_canvas, &k_imgSettings, nullptr, nullptr);


static std::vector<Widget*> s_selectableWidgets = 
{ 
	&s_targetWidget, 
	&s_timerWidget,
	&s_graphIconWidget,
	&s_limitsIconWidget,
	&s_settingsIconWidget
};
static size_t s_highlightedWidgetIndex = 0;
static bool s_isWidgetSelected = false;

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

void setUnitValue(ValueWidget& widget, float value, uint8_t decimalsMacro, float maxMacro, uint8_t decimalsMicro, float maxMicro, const char* unitSI)
{
	char buf[8];
	if (value >= 1000000.f)
	{
		widget.setValue(value / 1000000.f);
		widget.setDecimals(decimalsMacro);
		widget.setRange(0, maxMacro);
		sprintf(buf, "M%s", unitSI);
		widget.setSuffix(buf);
	}
	else if (value >= 1000.f)
	{
		widget.setValue(value / 1000.f);
		widget.setDecimals(decimalsMacro);
		widget.setRange(0, maxMacro);
		sprintf(buf, "k%s", unitSI);
		widget.setSuffix(buf);
	}
	else if (value >= 1.f)
	{
		widget.setValue(value);
		widget.setDecimals(decimalsMacro);
		widget.setRange(0, maxMacro);
		widget.setSuffix(unitSI);
	}	
	else
	{
		widget.setValue(value * 1000.f);
		widget.setDecimals(decimalsMicro);
		widget.setRange(0, maxMicro);
		sprintf(buf, "m%s", unitSI);
		widget.setSuffix(buf);
	}	
}

void processMeasurementTopUI()
{
	Measurement::TrackingMode mode = s_measurement.getTrackingMode();

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
		s_currentWidget.setTextColor(0x0);
		setUnitValue(s_targetWidget, s_measurement.getTargetCurrent(), 3, 99.f, 0, 999.999f, "A");
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
		setUnitValue(s_targetWidget, s_measurement.getTargetPower(), 3, 999.999f, 0, 999.999f, "W");
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
		setUnitValue(s_targetWidget, s_measurement.getTargetResistance(), 3, 999.999f, 0, 999.999f, "{");
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
	//s_timerWidget2.render();

	{
		int16_t border = 3;
		Widget::Position tlp = s_targetWidget.getPosition(Widget::Anchor::TopLeft).move(-k_imgTarget.width - border, 0);
		Widget::Position blp = s_targetWidget.getPosition(Widget::Anchor::BottomLeft);
		s_canvas.fillRoundRect(tlp.x - border, tlp.y - border, 1000, blp.y - tlp.y + border*2, border, trackedColor);
		s_canvas.fillRect(s_canvas.width() - 10, trackedBorderY, 1000, blp.y - trackedBorderY, trackedColor);

		s_canvas.drawRGBA8888Bitmap(tlp.x, (tlp.y + blp.y)/2 - k_imgTarget.height/2, 
								(const uint32_t*)k_imgTarget.pixel_data, k_imgTarget.width, k_imgTarget.height);

		//recenter the target widget with the icon
		Widget::Position parentPosition = s_resistanceWidget.getPosition(Widget::Anchor::BottomCenter);
		parentPosition.y = s_targetWidget.getPosition(Widget::Anchor::TopCenter).y; //no change in the y
		parentPosition.x += k_imgTarget.width / 2 + border;
  		s_targetWidget.setPosition(parentPosition, Widget::Anchor::TopCenter);
	}
	s_targetWidget.render();

	const Image* img = s_measurement.is4WireEnabled() ? &k_img4Wire : &k_img2Wire;
	s_canvas.drawRGBA8888Bitmap(s_temperatureWidget.getPosition(Widget::Anchor::TopLeft).x - 36, 0, 
							(const uint32_t*)img->pixel_data, img->width, img->height);

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
	{	
		s_timerWidget2.setEditing(true);
		s_timerWidget2.setRange(0, 50);
		s_timerWidget2.setSuffix("A");
		s_timerWidget2.process(s_knob);
	}
*/
	{
		s_highlightedWidgetIndex += s_knob.encoderDelta();
		s_highlightedWidgetIndex %= s_selectableWidgets.size();
		Widget* sw = s_selectableWidgets[s_highlightedWidgetIndex];

		int16_t rectBorder = 3;
		Widget::Position tlp = sw->getPosition(Widget::Anchor::TopLeft);
		s_canvas.drawRoundRect(tlp.x - rectBorder, tlp.y - rectBorder, 
							   sw->getWidth() + rectBorder*2, sw->getHeight() + rectBorder*2, 
							   rectBorder, 0xFFFF);
		s_canvas.drawRGBA8888Bitmap(tlp.x + sw->getWidth() + 4, 
								    tlp.y + sw->getHeight()/2 - k_imgKnob.height/2, 
									(const uint32_t*)k_imgKnob.pixel_data, 
									k_imgKnob.width, k_imgKnob.height);
	}
	{


		s_graphIconWidget.render();
		s_limitsIconWidget.render();
		s_settingsIconWidget.render();
	}

/*
	static int fanSpeed = 0;
	fanSpeed += s_knob.encoderChangedAcc();
	fanSpeed = std::min(std::max(fanSpeed, 0), 65535);
	printf("\nDAC: %d, temp: %f", fanSpeed, s_temperature);
	//setFanPWM(fanSpeed);
	setDAC(fanSpeed / 65535.f);
*/
}

void initMeasurementTopUI()
{
	int16_t xSpacing = 12;
	int16_t ySpacing = 6;

	int16_t column1X = s_canvas.width() / 4;
	int16_t column2X = s_canvas.width() * 3 / 4;

	//1st ROW
	s_voltageWidget.setUseContentHeight(true);
	s_voltageWidget.setRange(0, 99);
	s_voltageWidget.setTextColor(k_voltageColor);
	s_voltageWidget.setValueFont(&SansSerif_bold_32);
	s_voltageWidget.setSuffixFont(&SansSerif_bold_10);
  	s_voltageWidget.setDecimals(3);
  	s_voltageWidget.setPosition(Widget::Position{column1X, s_windowY + 8}, Widget::Anchor::TopCenter);

	s_currentWidget.setUseContentHeight(true);
	s_currentWidget.setRange(0, 99);
	s_currentWidget.setTextColor(k_currentColor);
	s_currentWidget.setValueFont(&SansSerif_bold_32);
	s_currentWidget.setSuffixFont(&SansSerif_bold_10);
  	s_currentWidget.setDecimals(3);
  	s_currentWidget.setPosition(Widget::Position{column2X, s_windowY + 8}, Widget::Anchor::TopCenter);

	//2nd ROW
	s_energyWidget.setUseContentHeight(true);
	s_energyWidget.setRange(0, 999.9999f);
	s_energyWidget.setTextColor(k_energyColor);
	s_energyWidget.setValueFont(&SansSerif_bold_18);
	s_energyWidget.setSuffixFont(&SansSerif_bold_10);
  	s_energyWidget.setDecimals(3);
  	s_energyWidget.setPosition(s_voltageWidget.getPosition(Widget::Anchor::BottomCenter).move(0, ySpacing), Widget::Anchor::TopCenter);

	s_powerWidget.setUseContentHeight(true);
	s_powerWidget.setRange(0, 999.9999f);
	s_powerWidget.setTextColor(k_powerColor);
	s_powerWidget.setValueFont(&SansSerif_bold_18);
	s_powerWidget.setSuffixFont(&SansSerif_bold_10);
  	s_powerWidget.setDecimals(3);
  	s_powerWidget.setPosition(s_currentWidget.getPosition(Widget::Anchor::BottomCenter).move(0, ySpacing), Widget::Anchor::TopCenter);

	//3rd ROW
	s_chargeWidget.setUseContentHeight(true);
	s_chargeWidget.setRange(0, 999.9999f);
	s_chargeWidget.setTextColor(k_chargeColor);
	s_chargeWidget.setValueFont(&SansSerif_bold_18);
	s_chargeWidget.setSuffixFont(&SansSerif_bold_10);
  	s_chargeWidget.setDecimals(3);
  	s_chargeWidget.setPosition(s_energyWidget.getPosition(Widget::Anchor::BottomCenter).move(0, ySpacing), Widget::Anchor::TopCenter);

	s_resistanceWidget.setUseContentHeight(true);
	s_resistanceWidget.setRange(0, 999.9999f);
	s_resistanceWidget.setTextColor(k_resistanceColor);
	s_resistanceWidget.setValueFont(&SansSerif_bold_18);
	s_resistanceWidget.setSuffixFont(&SansSerif_bold_10);
  	s_resistanceWidget.setDecimals(3);
  	s_resistanceWidget.setPosition(s_powerWidget.getPosition(Widget::Anchor::BottomCenter).move(0, ySpacing), Widget::Anchor::TopCenter);

  	//4th ROW
	s_timerWidget.setUseContentHeight(true);
	s_timerWidget.setTextColor(k_timerColor);
	s_timerWidget.setFont(&SansSerif_bold_18);
  	s_timerWidget.setPosition(s_chargeWidget.getPosition(Widget::Anchor::BottomCenter).move(0, ySpacing * 2), Widget::Anchor::TopCenter);

/*
	s_timerWidget2.setUseContentHeight(true);
	s_timerWidget2.setTextColor(k_timerColor);
	s_timerWidget2.setMainFont(&SansSerif_bold_18);
	s_timerWidget2.setSuffixFont(&SansSerif_bold_10);
  	s_timerWidget2.setPosition(Widget::Position{xSpacing, s_timerWidget.getPosition(Widget::Anchor::BottomLeft).y}.move(0, ySpacing * 2), Widget::Anchor::TopLeft);
*/
	s_targetWidget.setUseContentHeight(true);
	s_targetWidget.setRange(0, 999.9999f);
	s_targetWidget.setTextColor(0);
	s_targetWidget.setValueFont(&SansSerif_bold_18);
	s_targetWidget.setSuffixFont(&SansSerif_bold_10);
  	s_targetWidget.setDecimals(3);
  	s_targetWidget.setPosition(s_resistanceWidget.getPosition(Widget::Anchor::BottomCenter).move(0, ySpacing * 2), Widget::Anchor::TopCenter);
  	//s_targetWidget.setPosition(Widget::Position{s_canvas.width() - xSpacing, s_chargeWidget.getPosition(Widget::Anchor::BottomLeft).y}.move(0, ySpacing), Widget::Anchor::TopRight);


	s_modeWidget.setFont(&SansSerif_bold_13);
	s_modeWidget.setPosition(Widget::Position{0, s_windowY - 3});

	int16_t y = s_canvas.height() - s_graphIconWidget.getHeight() - s_limitsIconWidget.getHeight() - s_settingsIconWidget.getHeight();
	s_graphIconWidget.setPosition(Widget::Position(s_canvas.width(), y), Widget::Anchor::TopRight);
	y += s_graphIconWidget.getHeight();
	s_limitsIconWidget.setPosition(Widget::Position(s_canvas.width(), y), Widget::Anchor::TopRight);
	y += s_limitsIconWidget.getHeight();
	s_settingsIconWidget.setPosition(Widget::Position(s_canvas.width(), y), Widget::Anchor::TopRight);

}

