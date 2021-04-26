#pragma once

#include "Adafruit_GFX.h"
#include "WidgetBase.h"

class ValueWidget : public WidgetBase
{
public:
	ValueWidget(JLMBackBuffer& gfx, float value, const char* suffix);
	void setValueFont(const GFXfont* font);
	void setSuffixFont(const GFXfont* font);
	void setSuffix(const char* suffix);
	void setRange(float min, float max);
	void setDecimals(uint8_t decimals);
	void setTextColor(uint16_t color);
	void setMainColor(uint16_t color);
	void setSuffixColor(uint16_t color);
	void setUseContentHeight(bool enabled);
	void setUsePadding(bool enabled);

	enum class HorizontalAlignment
	{
		Left,
		Right
	};
	void setHorizontalAlignment(HorizontalAlignment alignment);

	int16_t getWidth() const override;
	int16_t getHeight() const override;
	void render() override;
	void setValue(float value);
	float getValue() const;

private:
	void updateGeometry() const;
	void valueToString(char* str) const;

	const GFXfont* m_valueFont = nullptr;
	const GFXfont* m_suffixFont = nullptr;

	std::string m_suffix;
	enum DirtyFlags
	{
		DirtyFlagGeometry = 1 << 0
	};
	mutable uint8_t m_dirtyFlags = 0xFF;
	uint8_t m_decimals = 3;
	mutable int16_t m_w = 0;
	mutable int16_t m_h = 0;
	mutable int16_t m_cw = 0;
	mutable int16_t m_ch = 0;
	bool m_useContentHeight = false;
	bool m_usePadding = false;
	HorizontalAlignment m_horizontalAlignment = HorizontalAlignment::Left;
	float m_value = 0;
	float m_min = -999999999.f;
	float m_max = 999999999.f;
	uint16_t m_mainColor = 0xFFFF;
	uint16_t m_suffixColor = 0xFFFF;
};
