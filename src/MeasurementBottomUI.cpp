#include "Measurement.h"
#include "MeasurementTopUI.h"
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
extern ValueWidget s_temperatureWidget;

extern LabelWidget s_modeWidget;
//static LabelWidget s_rangeWidget(s_canvas, "");
GraphWidget s_graphWidget(s_canvas, 512);

static uint8_t s_voltagePlot = 0;
static uint8_t s_currentPlot = 0;
static uint8_t s_powerPlot = 0;
static uint8_t s_resistancePlot = 0;
static uint8_t s_energyPlot = 0;
static uint8_t s_chargePlot = 0;

void processMeasurementBottomUI()
{
	float voltage = s_measurement.getVoltage();
	float current = s_measurement.getCurrent();
	float power = s_measurement.getPower();
	float resistance = s_measurement.getResistance();
	// float energy = s_measurement.getEnergy();
	// float charge = s_measurement.getCharge();

	Clock::duration loadTimer = s_measurement.getLoadTimer();

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
}

void initMeasurementBottomUI()
{
	int16_t ySpacing = 6;

	// int16_t column1X = s_canvas.width() / 4;
	// int16_t column2X = s_canvas.width() * 3 / 4;

	s_graphWidget.setPosition(Widget::Position{0, 150}.move(0, ySpacing * 3), Widget::Anchor::TopLeft);
	s_graphWidget.setSize(s_canvas.width(), s_canvas.height() - s_graphWidget.getPosition().y - 1);
	s_voltagePlot = s_graphWidget.addPlot("V", k_voltageColor, 0.1f);
	s_currentPlot = s_graphWidget.addPlot("A", k_currentColor, 0.1f);
	s_powerPlot = s_graphWidget.addPlot("W", k_powerColor, 0.1f);
	s_resistancePlot = s_graphWidget.addPlot("{", k_resistanceColor, 0.1f);
	s_energyPlot = s_graphWidget.addPlot("Wh", k_energyColor, 0.00000001f);
	s_chargePlot = s_graphWidget.addPlot("Ah", k_chargeColor, 0.00000001f);
}
