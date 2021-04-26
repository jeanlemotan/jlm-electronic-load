#pragma once

#include "Adafruit_GFX.h"
#include "WidgetBase.h"
#include "icons.h"

class ImageWidget : public WidgetBase
{
public:
	ImageWidget(JLMBackBuffer& gfx, const Image* image, const Image* highlighted, const Image* selected);

	int16_t getWidth() const override;
	int16_t getHeight() const override;
	void render() override;

private:
    const Image* m_image = nullptr;
    const Image* m_highlightedImage = nullptr;
    const Image* m_selectedImage = nullptr;
	mutable int16_t m_w = 0;
	mutable int16_t m_h = 0;
};
