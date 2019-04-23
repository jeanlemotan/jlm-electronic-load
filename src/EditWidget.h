#pragma once

#include "Adafruit_GFX.h"
#include "RotaryEncoder.h"
#include "Touchscreen.h"
#include "WidgetBase.h"

class EditWidget : public WidgetBase
{
public:
	EditWidget(Adafruit_GFX& gfx, const char* value, const char* suffix);

	void setValueFont(const GFXfont* font);
	void setSuffixFont(const GFXfont* font);

	void setSuffix(const char* suffix);

	void setTextColor(uint16_t color);
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
	void setValue(const char* value);
	const std::string& getValue() const;

    virtual void process(RotaryEncoder& knob);

private:
	void updateGeometry() const;
	void buildEditedValue(char* dst) const;

	const GFXfont* m_valueFont = nullptr;
	const GFXfont* m_suffixFont = nullptr;

	std::string m_suffix;
	std::string m_value;
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
	uint16_t m_selectedBackgroundColor = 0xFFFF;
	uint16_t m_selectedTextColor = 0x0;
	bool m_isEditing = false;
	int32_t m_selectedIndex = 0;
};
