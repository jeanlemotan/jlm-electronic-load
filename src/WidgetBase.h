#pragma once

#include "JLMBackBuffer.h"
#include "Widget.h"

class WidgetBase : public Widget
{
public:
	WidgetBase(JLMBackBuffer& gfx);

	void setPosition(const Position& position, Anchor anchor = Anchor::TopLeft) override;
	Position getPosition(Anchor anchor = Anchor::TopLeft) const override;

	void setSelected(bool selected) override;
	bool isSelected() const override;
	void setHighlighted(bool highlighted) override;
	bool isHighlighted() const override;

protected:
	Position computeBottomLeftPosition() const;

	JLMBackBuffer& m_gfx;
	Position m_position;
	Anchor m_anchor = Anchor::TopLeft;
	bool m_isSelected = false;
	bool m_isHighlighted = false;
};
