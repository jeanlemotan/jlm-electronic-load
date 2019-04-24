#include "ValueWidget.h"

ValueWidget::ValueWidget(Adafruit_GFX& gfx, float value, const char* suffix)
	: WidgetBase(gfx)
	, m_value(value)
{
	m_suffix = suffix ? suffix : "";
}
void ValueWidget::setValueFont(const GFXfont* font)
{
	m_valueFont = font;
	m_dirtyFlags |= DirtyFlagGeometry;
}
void ValueWidget::setSuffixFont(const GFXfont* font)
{
	m_suffixFont = font;
	m_dirtyFlags |= DirtyFlagGeometry;
}

void ValueWidget::setSuffix(const char* suffix)
{
	suffix = suffix ? suffix : "";
	if (m_suffix == suffix)
	{
		return;
	}
	m_suffix = suffix;
	m_dirtyFlags |= DirtyFlagGeometry;
}
void ValueWidget::setDecimals(uint8_t decimals)
{
	if (m_decimals != decimals)
	{
		m_decimals = decimals;
		m_dirtyFlags |= DirtyFlagGeometry;
	}	
}

void ValueWidget::setRange(float min, float max)
{
	m_min = min;
	m_max = max;
	setValue(m_value);
}
void ValueWidget::setTextColor(uint16_t color)
{
	setMainColor(color);
	setSuffixColor(color);
}
void ValueWidget::setMainColor(uint16_t color)
{
	if (m_mainColor != color)
	{
		m_mainColor = color;
	}
}
void ValueWidget::setSuffixColor(uint16_t color)
{
	if (m_suffixColor != color)
	{
		m_suffixColor = color;
	}
}

void ValueWidget::setUseContentHeight(bool enabled)
{
	if (m_useContentHeight != enabled)
	{
		m_useContentHeight = enabled;
		m_dirtyFlags |= DirtyFlagGeometry;
	}
}
void ValueWidget::setUsePadding(bool enabled)
{
	if (m_usePadding != enabled)
	{
		m_usePadding = enabled;
		m_dirtyFlags |= DirtyFlagGeometry;
	}
}
void ValueWidget::setHorizontalAlignment(HorizontalAlignment alignment)
{
	m_horizontalAlignment = alignment;
}
int16_t ValueWidget::getWidth() const
{
	updateGeometry();
	return m_w;
}
int16_t ValueWidget::getHeight() const
{
	updateGeometry();
	return m_useContentHeight ? m_ch : m_h;
}
void ValueWidget::render()
{
	updateGeometry();

	const GFXfont* oldFont = m_gfx.getFont();

	m_gfx.setTextColor(m_mainColor);
	m_gfx.setFont(m_valueFont);

	char str[16];
	valueToString(str);

	Position position = computeBottomLeftPosition();
	int16_t x = m_horizontalAlignment == HorizontalAlignment::Left ? position.x : position.x + m_w - m_cw;
	//printf("X: '%s', %d, %d, %d\n", str, m_x, x, m_gfx.getTextWidth(str));
	m_gfx.setCursor(x, position.y);
	m_gfx.print(str);
	if (!m_suffix.empty())
	{
		m_gfx.setTextColor(m_suffixColor);
		m_gfx.setFont(m_suffixFont);
		m_gfx.print(m_suffix.c_str());
	}

	m_gfx.setFont(oldFont);
}
void ValueWidget::setValue(float value)
{
	value = min(value, m_max);
	value = max(value, m_min);

	//clamp the unused decimals
	float p[] = { 1.f, 10.f, 100.f, 1000.f, 10000.f, 100000.f, 1000000.f, 10000000.f, 100000000.f };
	value = (float)((long long)(value * p[m_decimals + 1] + 0.5f)) / p[m_decimals + 1];

	if (m_value != value)
	{
		m_value = value;
		m_dirtyFlags |= DirtyFlagGeometry;
	}
}
float ValueWidget::getValue() const
{
	return m_value;
}

void ValueWidget::valueToString(char* str) const
{
	if (m_decimals == 0)
	{
		sprintf(str, "%d", (int32_t)m_value);
	}
	else
	{
		dtostrf(m_value, 0, m_decimals, str);
	}
}

void ValueWidget::updateGeometry() const
{
	if ((m_dirtyFlags & DirtyFlagGeometry) == 0)
	{
		return;
	}
	m_dirtyFlags &= ~DirtyFlagGeometry;

	const GFXfont* oldFont = m_gfx.getFont();

	char str[16] = { '\0' };
	size_t offset = 0;

	if (m_usePadding)
	{
		float maxAbsValue = std::max(m_max, std::abs(m_min));
		{
			int usedDigits = std::max((int)std::log10(std::abs(m_value)), 0) + 1;
			int maxDigits = std::max((int)std::log10(maxAbsValue), 0) + 1;
			if (maxDigits > usedDigits)
			{
				for (int i = 0; i < maxDigits - usedDigits; i++)
				{
					str[offset++] = '0';
				}
			}
		}
		if (m_value < 0 && m_min < 0) //reserve space for '-' if the value can also go negative
		{
			str[0] = '-';
			offset++;
		}
	}

	size_t contentOffset = offset;
	valueToString(str + offset);
	int16_t x, y;
	uint16_t w, h;
	m_gfx.setFont(m_valueFont);
	m_gfx.getTextBounds(str + contentOffset, 0, 0, &x, &y, &w, &h);
	m_cw = x + w;
	m_ch = h;

	m_gfx.getTextBounds(str, 0, 0, &x, &y, &w, &h);
	m_w = x + w;
	m_h = h;

	if (m_valueFont)
	{
		m_h = m_valueFont->yAdvance;
	}
	if (!m_suffix.empty())
	{
		m_gfx.setFont(m_suffixFont);
		m_gfx.getTextBounds(m_suffix.c_str(), 0, 0, &x, &y, &w, &h);
		m_ch = std::max<int16_t>(m_ch, h);
		m_cw += x + w;
		m_w += x + w;
		if (m_suffixFont)
		{
			m_h = std::max<int16_t>(m_h, m_suffixFont->yAdvance);
		}
	}
	m_gfx.setFont(oldFont);
}
