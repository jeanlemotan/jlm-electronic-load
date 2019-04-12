#pragma once

#include "Adafruit_GFX.h"
#include "Widget.h"

class LabelWidget : public Widget
{
public:
	LabelWidget(Adafruit_GFX& gfx, const char* value);
	void setFont(const GFXfont* font);
	void setTextColor(uint16_t color);
	void setTextScale(uint8_t scale);
	void setUseContentHeight(bool enabled);
	void setPosition(const Position& position, Anchor anchor = Anchor::TopLeft) override;
	Position getPosition(Anchor anchor = Anchor::TopLeft) const override;
	int16_t getWidth() const override;
	int16_t getHeight() const override;
	void render() override;
	void setSelected(bool selected) override;
	bool isSelected() const override;
	void setValue(const char* value);

private:
	void updateGeometry() const;

	Adafruit_GFX& m_gfx;
	const GFXfont* m_font = nullptr;

	char m_value[32] = { 0 };
	enum DirtyFlags
	{
		DirtyFlagGeometry = 1 << 0
	};
	mutable uint8_t m_dirtyFlags = 0xFF;
	uint8_t m_textScale = 1;
	Position m_position;
	mutable int16_t m_w = 0;
	mutable int16_t m_h = 0;
	mutable int16_t m_ch = 0;
	bool m_useContentHeight = false;
	bool m_isSelected = false;
	uint16_t m_textColor = 0xFFFF;
};
