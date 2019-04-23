#include "EditWidget.h"

EditWidget::EditWidget(Adafruit_GFX& gfx, const char* value, const char* suffix)
	: WidgetBase(gfx)
{
	m_value = value ? value : "";
	m_suffix = suffix ? suffix : "";
}

void EditWidget::setValueFont(const GFXfont* font)
{
	m_valueFont = font;
	m_dirtyFlags |= DirtyFlagGeometry;
}
void EditWidget::setSuffixFont(const GFXfont* font)
{
	m_suffixFont = font;
	m_dirtyFlags |= DirtyFlagGeometry;
}

void EditWidget::setSuffix(const char* suffix)
{
	suffix = suffix ? suffix : "";
	if (m_suffix == suffix)
	{
		return;
	}
	m_suffix = suffix;
	m_dirtyFlags |= DirtyFlagGeometry;
}

void EditWidget::setTextColor(uint16_t color)
{
	if (m_textColor != color)
	{
		m_textColor = color;
	}
}

void EditWidget::setSuffixColor(uint16_t color)
{
	if (m_suffixColor != color)
	{
		m_suffixColor = color;
	}
}

void EditWidget::setUseContentHeight(bool enabled)
{
	if (m_useContentHeight != enabled)
	{
		m_useContentHeight = enabled;
		m_dirtyFlags |= DirtyFlagGeometry;
	}
}

int16_t EditWidget::getWidth() const
{
	updateGeometry();
	return m_w;
}

int16_t EditWidget::getHeight() const
{
	updateGeometry();
	return m_useContentHeight ? m_ch : m_h;
}

void EditWidget::setSelectedBackgroundColor(uint16_t color)
{
	m_selectedBackgroundColor = color;
}
void EditWidget::setSelectedTextColor(uint16_t color)
{
	m_selectedTextColor = color;
}
void EditWidget::setEditing(bool enabled)
{
	m_isEditing = enabled;
}
bool EditWidget::isEditing() const
{
	return m_isEditing;
}
void EditWidget::setSelectedIndex(int32_t index)
{
	m_selectedIndex = index;
}
int32_t EditWidget::getSelectedIndex() const
{
	return m_selectedIndex;
}
char EditWidget::getSelectedChar() const
{
	if (m_selectedIndex >= 0 && m_selectedIndex < m_value.size())
	{
		return m_value[m_selectedIndex];
	}
	return 0;
}

void EditWidget::render()
{
	updateGeometry();

	const GFXfont* oldFont = m_gfx.getFont();

	m_gfx.setTextColor(m_textColor);
	m_gfx.setFont(m_valueFont);

	Position position = computeBottomLeftPosition();

	m_gfx.setCursor(position.x, position.y);
	if (m_isEditing)
	{
		char editedValue[128];
		buildEditedValue(editedValue);
		m_gfx.print(editedValue);
		m_gfx.setTextBgEnabled(false);
	}
	else
	{
		m_gfx.print(m_value.c_str());
	}
	if (!m_suffix.empty())
	{
		m_gfx.setTextColor(m_suffixColor);
		m_gfx.setFont(m_suffixFont);
		m_gfx.print(m_suffix.c_str());
	}	
	m_gfx.setFont(oldFont);
}

void EditWidget::buildEditedValue(char* dst) const
{
	dst[0] = '\0';
	//turn background off (just in case)
	//set the background color
	sprintf(dst, "#b-#b%04X", m_selectedBackgroundColor);
	char* dstPtr = dst + strlen(dst);

	for (size_t i = 0; i < m_value.size(); i++)
	{
		char ch = m_value[i];
		if (i == m_selectedIndex)
		{
			//turn background on
			//st text color to the selected one
			//add the char
			//turn background off
			//revert the text color to the normal one
			sprintf(dstPtr, "#b+#f%04X%c#b-#f%04X", m_selectedTextColor, ch, m_textColor);
			dstPtr = dst + strlen(dst);
		}
		else
		{
			*dstPtr++ = ch;
			*dstPtr = '\0';
		}
	}
}

void EditWidget::setValue(const char* value)
{
	value = value ? value : "";
	if (m_value == value)
	{
		return;
	}
	m_value = value;
	m_dirtyFlags |= DirtyFlagGeometry;
}

const std::string& EditWidget::getValue() const
{
	return m_value;
}

void EditWidget::process(RotaryEncoder& knob)
{

}

void EditWidget::updateGeometry() const
{
	if ((m_dirtyFlags & DirtyFlagGeometry) == 0)
	{
		return;
	}
	m_dirtyFlags &= ~DirtyFlagGeometry;

	const GFXfont* oldFont = m_gfx.getFont();
	m_gfx.setFont(m_valueFont);

	int16_t x, y;
	uint16_t w, h;
	m_gfx.getTextBounds(m_value.c_str(), 0, 0, &x, &y, &w, &h);
	m_w = w;
	m_ch = h;
	m_h = h;
	if (m_valueFont)
	{
		m_h = m_valueFont->yAdvance;
	}
	if (m_suffix[0] != '\0')
	{
		m_gfx.setFont(m_suffixFont);
		m_gfx.getTextBounds(m_suffix.c_str(), 0, 0, &x, &y, &w, &h);
		m_ch = std::max<int16_t>(m_ch, h);
		m_w += x + w;
		if (m_suffixFont)
		{
			m_h = std::max<int16_t>(m_h, m_suffixFont->yAdvance);
		}
	}	
	m_gfx.setFont(oldFont);
}
