#include "WidgetBase.h"

WidgetBase::WidgetBase(JLMBackBuffer& gfx)
	: m_gfx(gfx)
{
}
void WidgetBase::setPosition(const Position& position, Anchor anchor)
{
	m_position = position;
	m_anchor = anchor;
}
Widget::Position WidgetBase::computeBottomLeftPosition() const
{
	int16_t x = m_position.x;
	int16_t y = m_position.y;
	int16_t w = getWidth();
	int16_t h = getHeight();
	switch (m_anchor)
	{
		case Anchor::TopLeft: 
		y += h;
		break;
		case Anchor::TopRight: 
		x -= w;
		y += h;
		break;
		case Anchor::BottomLeft: 
		break;
		case Anchor::BottomRight: 
		x -= w;
		break;
		case Anchor::TopCenter: 
		x -= w / 2;
		y += h;
		break;
		case Anchor::BottomCenter: 
		x -= w / 2;
		break;
		case Anchor::CenterLeft: 
		y += h / 2;
		break;
		case Anchor::CenterRight: 
		x -= w;
		y += h / 2;
		break;
		case Anchor::Center:
		x -= w / 2;
		y += h / 2;
		break;
	}
	return Position(x, y);
}
Widget::Position WidgetBase::getPosition(Anchor anchor) const
{
	Position position = computeBottomLeftPosition();
	int16_t w = getWidth();
	int16_t h = getHeight();
	switch (anchor)
	{
		case Anchor::TopLeft: 		return Position{position.x, position.y - h};
		case Anchor::TopRight: 		return Position{position.x + w, position.y - h};
		case Anchor::BottomLeft: 	return Position{position.x, position.y};
		case Anchor::BottomRight: 	return Position{position.x + w, position.y};
		case Anchor::TopCenter: 	return Position{position.x + w / 2, position.y - h};
		case Anchor::BottomCenter: 	return Position{position.x + w / 2, position.y};
		case Anchor::CenterLeft: 	return Position{position.x, position.y - h / 2};
		case Anchor::CenterRight: 	return Position{position.x + w, position.y - h / 2};
		case Anchor::Center:		return Position{position.x + w / 2, position.y - h / 2};
	}	
	return position;
}

void WidgetBase::setSelected(bool selected)
{
    m_isSelected = selected;
}
bool WidgetBase::isSelected() const
{
    return m_isSelected;
}

void WidgetBase::setHighlighted(bool highlighted)
{
    m_isHighlighted = highlighted;
}
bool WidgetBase::isHighlighted() const
{
    return m_isHighlighted;
}
