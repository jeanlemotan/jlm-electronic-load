#pragma once

#include "Adafruit_GFX.h"
#include "RotaryEncoder.h"
#include "Touchscreen.h"
#include "WidgetBase.h"

class EditWidget : public WidgetBase
{
public:
	EditWidget(JLMBackBuffer& gfx, const char* string, const char* suffix);

	void setMainFont(const GFXfont* font);
	void setSuffixFont(const GFXfont* font);

	void setSuffix(const char* suffix);

	void setTextColor(uint16_t color);
	void setEditedBackgroundColor(uint16_t color);
	void setEditedTextColor(uint16_t color);
	void setSelectedBackgroundColor(uint16_t color);
	void setSelectedTextColor(uint16_t color);
	void setSuffixColor(uint16_t color);

	void setEditing(bool enabled);
	bool isEditing() const;

	void setSelectedIndex(int32_t index);
	int32_t getSelectedIndex() const;
	char getSelectedChar() const;

	void setUseContentHeight(bool enabled);

	int16_t getWidth() const override;
	int16_t getHeight() const override;
	void render() override;

	enum class Result
	{
		None,
		Ok,
		Cancel
	};

    virtual Result process(RotaryEncoder& knob);

protected:
	void setString(const char* string);
	const std::string& getString() const;

	void setEditingDigit(bool enabled);
	bool isEditingDigit() const;


private:
	void updateGeometry() const;
	void buildEditedString(char* dst) const;

	const GFXfont* m_mainFont = nullptr;
	const GFXfont* m_suffixFont = nullptr;

	std::string m_suffix;
	std::string m_string;
	enum DirtyFlags
	{
		DirtyFlagGeometry = 1 << 0
	};
	mutable uint8_t m_dirtyFlags = 0xFF;
	mutable int16_t m_w = 0;
	mutable int16_t m_h = 0;
	mutable int16_t m_ch = 0;
	bool m_useContentHeight = false;
	uint16_t m_suffixColor = 0xFFFF;
	uint16_t m_textColor = 0xFFFF;
	uint16_t m_editedBackgroundColor = 0xAAAA;
	uint16_t m_editedTextColor = 0x0;
	uint16_t m_selectedBackgroundColor = 0xFFFF;
	uint16_t m_selectedTextColor = 0x0;
	bool m_isEditing = false;
	bool m_isEditingDigit = false;
	int32_t m_selectedIndex = 0;
};
