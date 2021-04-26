#include "GraphWidget.h"

GraphWidget::GraphWidget(JLMBackBuffer& gfx, size_t capacity)
	: WidgetBase(gfx)
	, m_capacity(std::max(capacity, 50u))
{
}
uint8_t GraphWidget::addPlot(const char* name, uint16_t color, float minRange)
{
	Plot plot;
	plot.name = name;
	plot.color = color;
	plot.minRange = minRange;
	plot.points.reserve(m_capacity);
	m_plots.push_back(std::move(plot));
	return m_plots.size() - 1;
}
void GraphWidget::setPlotVisible(uint8_t plotIndex, bool visible)
{
	if (plotIndex >= m_plots.size())
	{
		assert(false && "Plot out of bounds");
		return;
	}
	Plot& plot = m_plots[plotIndex];
	plot.isVisible = visible;
}
bool GraphWidget::isPlotVisible(uint8_t plotIndex) const
{
	if (plotIndex >= m_plots.size())
	{
		assert(false && "Plot out of bounds");
		return false;
	}
	const Plot& plot = m_plots[plotIndex];
	return plot.isVisible;
}

void GraphWidget::resample(Plot& plot)
{
	plot.timePerUnit *= 2;
	//printf("\nResampling to %d", (int)plot.timePerUnit);
	size_t step = 0;
	for (auto it = plot.points.begin(); it != plot.points.end();)
	{
		if ((step & 1) == 1)
		{
			it = plot.points.erase(it);
		}
		else
		{
			++it;
		}
		step++;
	}
}
void GraphWidget::addValue(uint8_t plotIndex, Clock::duration timestamp, float value, bool continued)
{
	if (plotIndex >= m_plots.size())
	{
		assert(false && "Plot out of bounds");
		return;
	}
	Plot& plot = m_plots[plotIndex];
	if (plot.points.empty())
	{
		plot.firstTimestamp = timestamp;
		plot.lastTimestamp = timestamp;
		plot.minValue = value;
		plot.maxValue = value;
		plot.points.push_back(Point(value, continued));
		return;
	}

	if (timestamp <= plot.lastTimestamp)
	{
		return;
	}

	float lastValue = plot.points.back().value;
	float dv = value - lastValue;
	Clock::duration lastTimestamp = plot.lastTimestamp;
	Clock::duration dt = timestamp - lastTimestamp;

	//sample
	//printf("\nS: ");
	Clock::duration t = plot.timePerUnit;
	while (t < dt)
	{
		if (plot.points.size() >= m_capacity)
		{
			resample(plot);
		}
		assert(plot.points.size() < m_capacity);

		float sampledValue = lastValue + (std::chrono::duration_cast<secondsf>(t).count() / std::chrono::duration_cast<secondsf>(dt).count()) * dv;
		//printf("T:%d/%f, ", (int)(lastTimestamp + t), sampledValue);

		plot.minValue = std::min(plot.minValue, sampledValue);
		plot.maxValue = std::max(plot.maxValue, sampledValue);
		plot.points.push_back(Point(sampledValue, continued));
		plot.lastTimestamp = lastTimestamp + t;

		t += plot.timePerUnit;
	}
}

void GraphWidget::setSize(int16_t width, int16_t height)
{
	m_w = std::max<int16_t>(width, 1);
	m_h = std::max<int16_t>(height, 1);
}
int16_t GraphWidget::getWidth() const
{
	return m_w;
}
int16_t GraphWidget::getHeight() const
{
	return m_h;
}
void GraphWidget::clear()
{
	Plot cleanPlot;
	for (Plot& plot: m_plots)
	{
		plot.timePerUnit = cleanPlot.timePerUnit;
		plot.firstTimestamp = cleanPlot.firstTimestamp;
		plot.lastTimestamp = cleanPlot.lastTimestamp;
		plot.minValue = cleanPlot.minValue;
		plot.maxValue = cleanPlot.maxValue;
		plot.points.clear();
	}
}
void GraphWidget::render()
{
	Position position = getPosition(Anchor::TopLeft);
	m_gfx.fillRect(position.x, position.y, m_w, m_h, 0x0861);

	//printf("\nV: ");
	for (const Plot& plot: m_plots)
	{
		if (!plot.isVisible)
		{
			continue;
		}
		float minValue = plot.minValue;
		float maxValue = plot.maxValue;
		float range = plot.maxValue - plot.minValue;
		float minRange = std::max(plot.minRange, 0.00000001f);
		if (range < minRange)
		{
			float center = (maxValue + minValue) / 2.f;
			minValue = center - minRange / 2.f;
			maxValue = center + minRange / 2.f;
		}
		range = maxValue - minValue;
		float invRange = 1.f / range;
		int16_t index = 0;
		int16_t lastX, lastY;
		size_t count = std::max(plot.points.size(), 50u);
		for (const Point& point: plot.points)
		{
			//printf("%f, ", point.value);
			int16_t x = position.x + index * m_w / count;
			int16_t y = position.y + m_h - int16_t(((point.value - minValue) * invRange) * m_h);
			if (index > 0)
			{
				m_gfx.drawAALine(lastX, lastY, x, y, plot.color);
			}
			lastX = x;
			lastY = y;
			index++;
		}
	}
}
