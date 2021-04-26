#include "ImageWidget.h"

ImageWidget::ImageWidget(JLMBackBuffer& gfx, const Image* image, const Image* highlighted, const Image* selected)
    : WidgetBase(gfx)
    , m_image(image)
    , m_highlightedImage(highlighted)
    , m_selectedImage(selected)
{
    m_w = m_image->width;
    m_h = m_image->height;
    if (m_highlightedImage)
    {
        m_w = std::max(m_w, int16_t(m_highlightedImage->width));
        m_h = std::max(m_h, int16_t(m_highlightedImage->height));
    }
    if (m_selectedImage)
    {
        m_w = std::max(m_w, int16_t(m_selectedImage->width));
        m_h = std::max(m_h, int16_t(m_selectedImage->height));
    }
}

int16_t ImageWidget::getWidth() const
{
    return m_w;
}
int16_t ImageWidget::getHeight() const
{
    return m_h;
}
void ImageWidget::render()
{
    const Image* image = m_image;
    if (isSelected() && m_selectedImage)
    {
        image = m_selectedImage;
    }
    if (isHighlighted() && m_highlightedImage)
    {
        image = m_highlightedImage;
    }
    Position p = getPosition(Anchor::Center);
    m_gfx.drawRGBA8888Bitmap(p.x - image->width / 2, p.y - image->height / 2, 
                                (const uint32_t*)image->pixel_data, 
                                image->width, image->height);
}


