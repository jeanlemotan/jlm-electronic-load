#include "LabelWidget.h"

LabelWidget::LabelWidget(Adafruit_GFX& gfx, const char* value)
	: m_gfx(gfx)
{
	if (value)
	{
		strncpy(m_value, value, sizeof(m_value));
	}
}
void LabelWidget::setFont(const GFXfont* font)
{
	m_font = font;
	m_dirtyFlags |= DirtyFlagGeometry;
}
void LabelWidget::setTextColor(uint16_t color)
{
	if (m_textColor != color)
	{
		m_textColor = color;
	}
}

void LabelWidget::setTextScale(uint8_t scale)
{
	if (m_textScale != scale)
	{
		m_textScale = scale;
		m_dirtyFlags |= DirtyFlagGeometry;
	}
}
void LabelWidget::setUseContentHeight(bool enabled)
{
	if (m_useContentHeight != enabled)
	{
		m_useContentHeight = enabled;
		m_dirtyFlags |= DirtyFlagGeometry;
	}
}
void LabelWidget::setPosition(const Position& position, Anchor anchor)
{
	int16_t x = position.x;
	int16_t y = position.y;
	int16_t w = getWidth();
	int16_t h = getHeight();
	switch (anchor)
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

	if (m_position.x != x || m_position.y != y)
	{
		m_position.x = x;
		m_position.y = y;
	}
}
int16_t LabelWidget::getWidth() const
{
	updateGeometry();
	return m_w;
}
int16_t LabelWidget::getHeight() const
{
	updateGeometry();
	return m_useContentHeight ? m_ch : m_h;
}
Widget::Position LabelWidget::getPosition(Anchor anchor) const
{
	int16_t w = getWidth();
	int16_t h = getHeight();
	switch (anchor)
	{
		case Anchor::TopLeft: 		return Position{m_position.x, m_position.y - h};
		case Anchor::TopRight: 		return Position{m_position.x + w, m_position.y - h};
		case Anchor::BottomLeft: 	return Position{m_position.x, m_position.y};
		case Anchor::BottomRight: 	return Position{m_position.x + w, m_position.y};
		case Anchor::TopCenter: 	return Position{m_position.x + w / 2, m_position.y - h};
		case Anchor::BottomCenter: 	return Position{m_position.x + w / 2, m_position.y};
		case Anchor::CenterLeft: 	return Position{m_position.x, m_position.y - h / 2};
		case Anchor::CenterRight: 	return Position{m_position.x + w, m_position.y - h / 2};
		case Anchor::Center:		return Position{m_position.x + w / 2, m_position.y - h / 2};
	}	
	return m_position;
}

void LabelWidget::render()
{
	const GFXfont* oldFont = m_gfx.getFont();
	m_gfx.setFont(m_font);
	m_gfx.setTextColor(m_textColor);
	m_gfx.setTextSize(m_textScale);
	m_gfx.setCursor(m_position.x, getPosition(Anchor::BottomLeft).y);
	m_gfx.print(m_value);
	m_gfx.setFont(oldFont);
}
void LabelWidget::setSelected(bool selected)
{
	if (m_isSelected != selected)
	{
		m_isSelected = selected;
	}
}
bool LabelWidget::isSelected() const
{
	return m_isSelected;
}
void LabelWidget::setValue(const char* value)
{
	if (value)
	{
		if (strcmp(value, m_value) == 0)
		{
			return;
		}
		strncpy(m_value, value, sizeof(m_value));
	}
	else
	{
		if (m_value[0] == '\0')
		{
			return;
		}
		m_value[0] = '\0';
	}

	m_dirtyFlags |= DirtyFlagGeometry;
}

void LabelWidget::updateGeometry() const
{
	if ((m_dirtyFlags & DirtyFlagGeometry) == 0)
	{
		return;
	}
	m_dirtyFlags &= ~DirtyFlagGeometry;

	const GFXfont* oldFont = m_gfx.getFont();
	m_gfx.setFont(m_font);

	int16_t x, y;
	uint16_t w, h;
	m_gfx.setTextSize(m_textScale);
	m_gfx.getTextBounds(m_value, 0, 0, &x, &y, &w, &h);
	m_w = w;
	m_ch = h;
	m_h = h;
	if (m_font)
	{
		m_h = m_font->yAdvance;
	}
	m_gfx.setFont(oldFont);
}
