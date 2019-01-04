#pragma once

#include <Adafruit_GFX.h>
#include "Widget.h"

class ValueWidget : public Widget
{
public:
	ValueWidget(Adafruit_GFX& gfx, float value, const char* prefix);
	void setSuffix(const char* prefix);
	void setLimits(float minLimit, float maxLimit);
	void setDecimals(uint8_t decimals);
	void setTextColor(uint16_t color);
	void setBackgroundColor(uint16_t color);
	void setTextScale(uint8_t scale);
	void setPosition(int16_t x, int16_t y) override;
	int16_t getX() const override;
	int16_t getY() const override;
	int16_t getWidth() const override;
	int16_t getHeight() const override;
	void update() override;
	void setSelected(bool selected) override;
	bool isSelected() const override;
	void setValue(float value);
	float getValue() const;

private:
	void updateGeometry() const;
	void valueToString(char* str) const;

	Adafruit_GFX& m_gfx;
	char m_suffix[8] = { 0 };
	enum DirtyFlags
	{
		DirtyFlagRender = 1 << 0,
		DirtyFlagGeometry = 1 << 1
	};
	mutable uint8_t m_dirtyFlags = 0xFF;
	uint8_t m_textScale = 1;
	uint8_t m_decimals = 3;
	int16_t m_x = 0;
	int16_t m_y = 0;
	mutable int16_t m_w = 0;
	mutable int16_t m_h = 0;
	float m_value = 0;
	float m_min = -999999999.f;
	float m_max = 999999999.f;
	bool m_isSelected = false;
	uint16_t m_textColor = 0xFFFF;
	uint16_t m_backgroundColor = 0x0;
};