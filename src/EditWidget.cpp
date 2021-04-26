#include "EditWidget.h"

EditWidget::EditWidget(JLMBackBuffer& gfx, const char* string, const char* suffix)
	: WidgetBase(gfx)
{
	m_string = string ? string : "";
	m_suffix = suffix ? suffix : "";
}

void EditWidget::setMainFont(const GFXfont* font)
{
	m_mainFont = font;
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

void EditWidget::setEditedBackgroundColor(uint16_t color)
{
	m_editedBackgroundColor = color;
}
void EditWidget::setEditedTextColor(uint16_t color)
{
	m_editedTextColor = color;
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
void EditWidget::setEditingDigit(bool enabled)
{
	m_isEditingDigit = enabled;
}
bool EditWidget::isEditingDigit() const
{
	return m_isEditingDigit;
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
	if (m_selectedIndex >= 0 && m_selectedIndex < m_string.size())
	{
		return m_string[m_selectedIndex];
	}
	return 0;
}

void EditWidget::render()
{
	updateGeometry();

	const GFXfont* oldFont = m_gfx.getFont();

	m_gfx.setTextColor(m_textColor);
	m_gfx.setFont(m_mainFont);

	Position position = computeBottomLeftPosition();

	m_gfx.setCursor(position.x, position.y);
	if (m_isEditing)
	{
		char editedString[128];
		buildEditedString(editedString);
		m_gfx.print(editedString);
		m_gfx.setTextBgEnabled(false);
	}
	else
	{
		m_gfx.print(m_string.c_str());
	}
	if (!m_suffix.empty())
	{
		m_gfx.setTextColor(m_suffixColor);
		m_gfx.setFont(m_suffixFont);
		m_gfx.print(m_suffix.c_str());
	}	
	m_gfx.setFont(oldFont);
}

void EditWidget::buildEditedString(char* dst) const
{
	dst[0] = '\0';
	//turn background off (just in case)
	//set the background color
	sprintf(dst, "#b-#b%04X", m_isEditingDigit ? m_editedBackgroundColor : m_selectedBackgroundColor);
	char* dstPtr = dst + strlen(dst);

	for (size_t i = 0; i < m_string.size(); i++)
	{
		char ch = m_string[i];
		if (i == m_selectedIndex)
		{
			//turn background on
			//st text color to the selected one
			//add the char
			//turn background off
			//revert the text color to the normal one
			sprintf(dstPtr, "#b+#f%04X%c#b-#f%04X", m_isEditingDigit ? m_editedTextColor : m_selectedTextColor, ch, m_textColor);
			dstPtr = dst + strlen(dst);
		}
		else
		{
			*dstPtr++ = ch;
			*dstPtr = '\0';
		}
	}
}

void EditWidget::setString(const char* string)
{
	string = string ? string : "";
	if (m_string == string)
	{
		return;
	}
	m_string = string;
	m_dirtyFlags |= DirtyFlagGeometry;
}

const std::string& EditWidget::getString() const
{
	return m_string;
}

EditWidget::Result EditWidget::process(RotaryEncoder& knob)
{
	return Result::None;
}

void EditWidget::updateGeometry() const
{
	if ((m_dirtyFlags & DirtyFlagGeometry) == 0)
	{
		return;
	}
	m_dirtyFlags &= ~DirtyFlagGeometry;

	const GFXfont* oldFont = m_gfx.getFont();
	m_gfx.setFont(m_mainFont);

	int16_t x, y;
	uint16_t w, h;
	m_gfx.getTextBounds(m_string.c_str(), 0, 0, &x, &y, &w, &h);
	m_w = w;
	m_ch = h;
	m_h = h;
	if (m_mainFont)
	{
		m_h = m_mainFont->yAdvance;
	}
	if (!m_suffix.empty())
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
