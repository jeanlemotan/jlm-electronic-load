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
	m_dirtyFlags |= DirtyFlagRender;
	m_dirtyFlags |= DirtyFlagGeometry;
}
void LabelWidget::setTextColor(uint16_t color)
{
	if (m_textColor != color)
	{
		m_textColor = color;
		m_dirtyFlags |= DirtyFlagRender;
	}
}

void LabelWidget::setTextScale(uint8_t scale)
{
	if (m_textScale != scale)
	{
		m_textScale = scale;
		m_dirtyFlags |= DirtyFlagRender;
		m_dirtyFlags |= DirtyFlagGeometry;
	}
}
void LabelWidget::setPosition(int16_t x, int16_t y)
{
	if (m_x != x || m_y != y)
	{
		m_x = x;
		m_y = y;
		m_dirtyFlags |= DirtyFlagRender;
		m_dirtyFlags |= DirtyFlagGeometry;
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
	return m_h;
}
int16_t LabelWidget::getX() const
{
	return m_x;
}
int16_t LabelWidget::getY() const
{
	return m_y;
}

void LabelWidget::render()
{
	if ((m_dirtyFlags & DirtyFlagRender) == 0)
	{
		//return;
	}
	m_dirtyFlags &= ~DirtyFlagRender;

	const GFXfont* oldFont = m_gfx.getFont();
	m_gfx.setFont(m_font);
	m_gfx.setTextColor(m_textColor);
	m_gfx.setTextSize(m_textScale);
	m_gfx.setCursor(m_x, m_y);
	m_gfx.print(m_value);
	m_gfx.setFont(oldFont);
}
void LabelWidget::setSelected(bool selected)
{
	if (m_isSelected != selected)
	{
		m_isSelected = selected;
		m_dirtyFlags |= DirtyFlagRender;
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

	m_dirtyFlags |= DirtyFlagRender;
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
	m_h = h;
	if (m_font)
	{
		m_h = m_font->yAdvance;
	}
	m_gfx.setFont(oldFont);
}
