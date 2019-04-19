#include "LabelWidget.h"

LabelWidget::LabelWidget(Adafruit_GFX& gfx, const char* value)
	: WidgetBase(gfx)
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
void LabelWidget::render()
{
	Position position = computeBottomLeftPosition();
	const GFXfont* oldFont = m_gfx.getFont();
	m_gfx.setFont(m_font);
	m_gfx.setTextColor(m_textColor);
	m_gfx.setTextSize(m_textScale);
	m_gfx.setCursor(position.x, position.y);
	m_gfx.print(m_value);
	m_gfx.setFont(oldFont);
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
