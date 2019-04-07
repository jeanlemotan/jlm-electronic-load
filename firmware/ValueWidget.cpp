#include "ValueWidget.h"

ValueWidget::ValueWidget(Adafruit_GFX& gfx, float value, const char* suffix)
	: m_gfx(gfx)
	, m_value(value)
{
	strcpy(m_suffix, suffix);
}
void ValueWidget::setValueFont(const GFXfont* font)
{
	m_valueFont = font;
	m_dirtyFlags |= DirtyFlagRender;
	m_dirtyFlags |= DirtyFlagGeometry;
}
void ValueWidget::setSuffixFont(const GFXfont* font)
{
	m_suffixFont = font;
	m_dirtyFlags |= DirtyFlagRender;
	m_dirtyFlags |= DirtyFlagGeometry;
}

void ValueWidget::setSuffix(const char* suffix)
{
	strcpy(m_suffix, suffix);
	m_dirtyFlags |= DirtyFlagRender;
	m_dirtyFlags |= DirtyFlagGeometry;
}
void ValueWidget::setDecimals(uint8_t decimals)
{
	if (m_decimals != decimals)
	{
		m_decimals = decimals;
		m_dirtyFlags |= DirtyFlagRender;
		m_dirtyFlags |= DirtyFlagGeometry;
	}	
}

void ValueWidget::setLimits(float minLimit, float maxLimit)
{
	m_min = minLimit;
	m_max = maxLimit;
	setValue(m_value);
}
void ValueWidget::setTextColor(uint16_t color)
{
	if (m_textColor != color)
	{
		m_textColor = color;
		m_dirtyFlags |= DirtyFlagRender;
	}
}

void ValueWidget::setTextScale(uint8_t scale)
{
	if (m_textScale != scale)
	{
		m_textScale = scale;
		m_dirtyFlags |= DirtyFlagRender;
		m_dirtyFlags |= DirtyFlagGeometry;
	}
}
void ValueWidget::setPosition(int16_t x, int16_t y)
{
	if (m_x != x || m_y != y)
	{
		m_x = x;
		m_y = y;
		m_dirtyFlags |= DirtyFlagRender;
		m_dirtyFlags |= DirtyFlagGeometry;
	}
}
int16_t ValueWidget::getWidth() const
{
	updateGeometry();
	return m_w;
}
int16_t ValueWidget::getHeight() const
{
	updateGeometry();
	return m_h;
}
int16_t ValueWidget::getX() const
{
	return m_x;
}
int16_t ValueWidget::getY() const
{
	return m_y;
}

void ValueWidget::update()
{
	const GFXfont* oldFont = m_gfx.getFont();

	if ((m_dirtyFlags & DirtyFlagRender) == 0)
	{
		//return;
	}
	m_dirtyFlags &= ~DirtyFlagRender;

	m_gfx.setTextColor(m_textColor);
	m_gfx.setTextSize(m_textScale);
	m_gfx.setFont(m_valueFont);

	char str[16];
	valueToString(str);

	int16_t x = m_x;
	if (m_value >= 0)
	{
		x += m_gfx.getCharWidth('-', true);
	}
	//printf("X: '%s', %d, %d, %d\n", str, m_x, x, m_gfx.getTextWidth(str));
	m_gfx.setCursor(x, m_y);
	m_gfx.print(str);
	if (m_suffix[0] != '\0')
	{
		m_gfx.setFont(m_suffixFont);
		m_gfx.print(m_suffix);
	}

	m_gfx.setFont(oldFont);
}
void ValueWidget::setSelected(bool selected)
{
	if (m_isSelected != selected)
	{
		m_isSelected = selected;
		m_dirtyFlags |= DirtyFlagRender;
	}
}
bool ValueWidget::isSelected() const
{
	return m_isSelected;
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
		m_dirtyFlags |= DirtyFlagRender;
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

	char str[16];
	if (m_value < 0)
	{
		valueToString(str);
	}
	else
	{
		str[0] = '-';
		valueToString(str + 1);	
	}
	int16_t x, y;
	uint16_t w, h;
	m_gfx.setTextSize(m_textScale);
	m_gfx.setFont(m_valueFont);
	m_gfx.getTextBounds(str, 0, 0, &x, &y, &w, &h);
	m_w = w;
	m_h = h;
	if (m_valueFont)
	{
		m_h = m_valueFont->yAdvance;
	}
	if (m_suffix[0] != '\0')
	{
		m_gfx.setFont(m_suffixFont);
		m_gfx.getTextBounds(m_suffix, 0, 0, &x, &y, &w, &h);
		m_w += w;
		if (m_suffixFont)
		{
			m_h = std::max<int16_t>(m_h, m_suffixFont->yAdvance);
		}
	}
	m_gfx.setFont(oldFont);
}
