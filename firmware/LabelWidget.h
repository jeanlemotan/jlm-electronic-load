#pragma once

#include "Adafruit_GFX.h"
#include "Widget.h"

class LabelWidget : public Widget
{
public:
	LabelWidget(Adafruit_GFX& gfx, const char* value);
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
	void setValue(const char* value);

private:
	void updateGeometry() const;

	Adafruit_GFX& m_gfx;
	char m_value[32] = { 0 };
	enum DirtyFlags
	{
		DirtyFlagRender = 1 << 0,
		DirtyFlagGeometry = 1 << 1
	};
	mutable uint8_t m_dirtyFlags = 0xFF;
	uint8_t m_textScale = 1;
	int16_t m_x = 0;
	int16_t m_y = 0;
	mutable int16_t m_w = 0;
	mutable int16_t m_h = 0;
	bool m_isSelected = false;
	uint16_t m_textColor = 0xFFFF;
	uint16_t m_backgroundColor = 0x0;
};
