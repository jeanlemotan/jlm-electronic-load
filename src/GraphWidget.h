#pragma once

#include "Adafruit_GFX.h"
#include "WidgetBase.h"
#include "Clock.h"

class GraphWidget : public WidgetBase
{
public:
	GraphWidget(JLMBackBuffer& gfx, size_t capacity);
	uint8_t addPlot(const char* name, uint16_t color, float minRange);
	void setPlotVisible(uint8_t plotIndex, bool visible);
	bool isPlotVisible(uint8_t plotIndex) const;

	void addValue(uint8_t plotIndex, Clock::duration timestamp, float value, bool continued);

	void setSize(int16_t width, int16_t height);
	int16_t getWidth() const override;
	int16_t getHeight() const override;

	void clear();
	void render() override;

private:
	size_t m_capacity = 100;

	struct Point
	{
		Point(float value, bool continued)
			: continued(continued)
			, value(value)
		{}
		bool continued;
		float value;
	};

	struct Plot
	{
		std::string name;
		uint16_t color = 0xFFFF;
		bool isVisible = true;
		float minRange = 0.00000001f;
		Clock::duration timePerUnit = std::chrono::milliseconds(100);
		Clock::duration firstTimestamp = Clock::duration::zero();
		Clock::duration lastTimestamp = Clock::duration::zero();
		float minValue = 999999;
		float maxValue = -999999;
		std::vector<Point> points;
	};

	void resample(Plot& plot);

	std::vector<Plot> m_plots;

	int16_t m_w = 0;
	int16_t m_h = 0;
};
