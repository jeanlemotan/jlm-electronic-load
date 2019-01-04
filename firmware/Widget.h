#pragma once

class Widget
{
public:
	virtual void update() = 0;
	virtual void setSelected(bool selected) = 0;
	virtual bool isSelected() const = 0;
	virtual int16_t getWidth() const = 0;
	virtual int16_t getHeight() const = 0;
	virtual void setPosition(int16_t x, int16_t y) = 0;
	virtual int16_t getX() const = 0;
	virtual int16_t getY() const = 0;
};