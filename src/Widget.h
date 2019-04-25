#pragma once

class Widget
{
public:
	enum class Anchor
	{
		TopLeft,
		TopRight,
		BottomLeft,
		BottomRight,
		TopCenter,
		BottomCenter,
		CenterLeft,
		CenterRight,
		Center
	};

	struct Position
	{
		Position() = default;
		Position(int x, int y) : x(int16_t(x)), y(int16_t(y)) {};
		int16_t x = 0;
		int16_t y = 0;
		Position& move(int16_t dx, int16_t dy) { x += dx; y += dy; return *this; }
		Position& alignX(const Position& p) { x = p.x; return *this; }
		Position& alignY(const Position& p) { y = p.y; return *this; }
	};

	virtual void render() = 0;
	virtual int16_t getWidth() const = 0;
	virtual int16_t getHeight() const = 0;
	virtual void setPosition(const Position& position, Anchor anchor = Anchor::TopLeft) = 0;
	virtual Position getPosition(Anchor anchor = Anchor::TopLeft) const = 0;
	virtual void setSelected(bool selected) = 0;
	virtual bool isSelected() const = 0;
	virtual void setHighlighted(bool highlighted) = 0;
	virtual bool isHighlighted() const = 0;
};