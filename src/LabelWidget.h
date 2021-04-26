#pragma once

#include "Adafruit_GFX.h"
#include "WidgetBase.h"

class LabelWidget : public WidgetBase
{
public:
	LabelWidget(JLMBackBuffer& gfx, const char* value);
	void setFont(const GFXfont* font);
	void setTextColor(uint16_t color);
	void setTextScale(uint8_t scale);
	void setUseContentHeight(bool enabled);
	int16_t getWidth() const override;
	int16_t getHeight() const override;
	void render() override;
	void setValue(const char* value);

private:
	void updateGeometry() const;

	const GFXfont* m_font = nullptr;

	std::string m_value;
	enum DirtyFlags
	{
		DirtyFlagGeometry = 1 << 0
	};
	mutable uint8_t m_dirtyFlags = 0xFF;
	uint8_t m_textScale = 1;
	mutable int16_t m_w = 0;
	mutable int16_t m_h = 0;
	mutable int16_t m_ch = 0;
	bool m_useContentHeight = false;
	uint16_t m_textColor = 0xFFFF;
};
