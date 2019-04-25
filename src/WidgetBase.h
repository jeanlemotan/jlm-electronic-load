#pragma once

#include "Adafruit_GFX.h"
#include "Widget.h"

class WidgetBase : public Widget
{
public:
	WidgetBase(Adafruit_GFX& gfx);

	void setPosition(const Position& position, Anchor anchor = Anchor::TopLeft) override;
	Position getPosition(Anchor anchor = Anchor::TopLeft) const override;

	void setSelected(bool selected) override;
	bool isSelected() const override;
	void setHighlighted(bool highlighted) override;
	bool isHighlighted() const override;

protected:
	Position computeBottomLeftPosition() const;

	Adafruit_GFX& m_gfx;
	Position m_position;
	Anchor m_anchor = Anchor::TopLeft;
	bool m_isSelected = false;
	bool m_isHighlighted = false;
};
