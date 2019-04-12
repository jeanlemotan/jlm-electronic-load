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
	m_dirtyFlags |= DirtyFlagGeometry;
}
void ValueWidget::setSuffixFont(const GFXfont* font)
{
	m_suffixFont = font;
	m_dirtyFlags |= DirtyFlagGeometry;
}

void ValueWidget::setSuffix(const char* suffix)
{
	strcpy(m_suffix, suffix);
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

void ValueWidget::setLimits(float minLimit, float maxLimit)
{
	m_min = minLimit;
	m_max = maxLimit;
	setValue(m_value);
}
void ValueWidget::setTextColor(uint16_t color)
{
	setValueColor(color);
	setSuffixColor(color);
}
void ValueWidget::setValueColor(uint16_t color)
{
	if (m_valueColor != color)
	{
		m_valueColor = color;
	}
}
void ValueWidget::setSuffixColor(uint16_t color)
{
	if (m_suffixColor != color)
	{
		m_suffixColor = color;
	}
}

void ValueWidget::setTextScale(uint8_t scale)
{
	if (m_textScale != scale)
	{
		m_textScale = scale;
		m_dirtyFlags |= DirtyFlagGeometry;
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
void ValueWidget::setPosition(const Position& position, Anchor anchor)
{
	m_position = position;
	m_anchor = anchor;
}
Widget::Position ValueWidget::computeBottomLeftPosition() const
{
	int16_t x = m_position.x;
	int16_t y = m_position.y;
	int16_t w = getWidth();
	int16_t h = getHeight();
	switch (m_anchor)
	{
		case Anchor::TopLeft: 
		y += h;
		break;
		case Anchor::TopRight: 
		x -= w;
		y += h;
		break;
		case Anchor::BottomLeft: 
		break;
		case Anchor::BottomRight: 
		x -= w;
		break;
		case Anchor::TopCenter: 
		x -= w / 2;
		y += h;
		break;
		case Anchor::BottomCenter: 
		x -= w / 2;
		break;
		case Anchor::CenterLeft: 
		y += h / 2;
		break;
		case Anchor::CenterRight: 
		x -= w;
		y += h / 2;
		break;
		case Anchor::Center:
		x -= w / 2;
		y += h / 2;
		break;
	}
	return Position(x, y);
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
Widget::Position ValueWidget::getPosition(Anchor anchor) const
{
	Position position = computeBottomLeftPosition();
	int16_t w = getWidth();
	int16_t h = getHeight();
	switch (anchor)
	{
		case Anchor::TopLeft: 		return Position{position.x, position.y - h};
		case Anchor::TopRight: 		return Position{position.x + w, position.y - h};
		case Anchor::BottomLeft: 	return Position{position.x, position.y};
		case Anchor::BottomRight: 	return Position{position.x + w, position.y};
		case Anchor::TopCenter: 	return Position{position.x + w / 2, position.y - h};
		case Anchor::BottomCenter: 	return Position{position.x + w / 2, position.y};
		case Anchor::CenterLeft: 	return Position{position.x, position.y - h / 2};
		case Anchor::CenterRight: 	return Position{position.x + w, position.y - h / 2};
		case Anchor::Center:		return Position{position.x + w / 2, position.y - h / 2};
	}	
	return position;
}


void ValueWidget::render()
{
	updateGeometry();

	const GFXfont* oldFont = m_gfx.getFont();

	m_gfx.setTextColor(m_valueColor);
	m_gfx.setTextSize(m_textScale);
	m_gfx.setFont(m_valueFont);

	char str[16];
	valueToString(str);

	Position position = computeBottomLeftPosition();
	int16_t x = m_horizontalAlignment == HorizontalAlignment::Left ? position.x : position.x + m_w - m_cw;
	//printf("X: '%s', %d, %d, %d\n", str, m_x, x, m_gfx.getTextWidth(str));
	m_gfx.setCursor(x, position.y);
	m_gfx.print(str);
	if (m_suffix[0] != '\0')
	{
		m_gfx.setTextColor(m_suffixColor);
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
	m_gfx.setTextSize(m_textScale);
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
	if (m_suffix[0] != '\0')
	{
		m_gfx.setFont(m_suffixFont);
		m_gfx.getTextBounds(m_suffix, 0, 0, &x, &y, &w, &h);
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
