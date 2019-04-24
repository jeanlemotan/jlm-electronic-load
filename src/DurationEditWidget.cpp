#include "DurationEditWidget.h"
#include "icons.h"

DurationEditWidget::DurationEditWidget(Adafruit_GFX& gfx, const char* suffix)
    : EditWidget(gfx, nullptr, suffix)
    , m_min(Clock::duration::zero())
    , m_max(std::chrono::hours(99))
{
    _setDuration(m_duration);
}
void DurationEditWidget::setRange(Clock::duration min, Clock::duration max)
{
    m_min = std::min(min, max);
    m_max = std::max(min, max);
    m_duration = std::max(std::min(m_duration, m_max), m_min);
}
void DurationEditWidget::setDuration(Clock::duration duration)
{
    duration = std::max(std::min(duration, m_max), m_min);
    if (m_duration == duration)
    {
        return;
    }
    m_duration = duration;
    _setDuration(m_duration);
}
void DurationEditWidget::_setDuration(Clock::duration duration)
{
    int32_t seconds = std::chrono::duration_cast<std::chrono::seconds>(m_duration).count() % 60;
    int32_t minutes = std::chrono::duration_cast<std::chrono::minutes>(m_duration).count() % 60;
    int32_t hours = std::chrono::duration_cast<std::chrono::hours>(m_duration).count();
    char buf[128];
    sprintf(buf, "%02d:%02d:%02d", hours, minutes, seconds);
    setString(buf);
}
Clock::duration DurationEditWidget::getDuration() const
{
    return m_duration;
}
EditWidget::Result DurationEditWidget::process(RotaryEncoder& knob)
{
    if (!isEditing())
    {
        setEditingDigit(false);
        return Result::None;
    }

    const std::string& v = getString();

    if (knob.buttonState() == RotaryEncoder::ButtonState::Released)
    {
        int32_t crtIndex = getSelectedIndex();
        if (crtIndex < v.size())
        {
            setEditingDigit(!isEditingDigit());
        }
        else if (crtIndex == v.size()) //OK
        {
            return Result::Ok;
        }
        else if (crtIndex == v.size() + 1) //Cancel
        {
            return Result::Cancel;
        }
    }

    if (!isEditingDigit())
    {
        int32_t delta = knob.encoderDelta();
        if (delta != 0 && v.size() > 1)
        {
            int32_t crtIndex = getSelectedIndex();
            do
            {
                crtIndex = std::max(std::min<int32_t>(crtIndex + delta, v.size() + 1), 0);
            } while (crtIndex < v.size() && !::isdigit(v[crtIndex]));
            setSelectedIndex(crtIndex);
        }
    }
    else
    {
        int32_t delta = knob.encoderDeltaAcc();
        size_t crtIndexFromBack = v.size() - getSelectedIndex() - 1;

        Clock::duration d = m_duration;
             if (crtIndexFromBack == 0) d += std::chrono::seconds(delta);
        else if (crtIndexFromBack == 1) d += std::chrono::seconds(delta * 10);
        else if (crtIndexFromBack == 3) d += std::chrono::minutes(delta);
        else if (crtIndexFromBack == 4) d += std::chrono::minutes(delta * 10);
        else if (crtIndexFromBack == 6) d += std::chrono::hours(delta);
        else if (crtIndexFromBack == 7) d += std::chrono::hours(delta * 10);

        setDuration(d);
    }
}

void DurationEditWidget::render()
{
    EditWidget::render();

    const std::string& v = getString();
    Position p = getPosition(Anchor::BottomRight).move(4, 0);
    int32_t crtIndex = getSelectedIndex();
    if (crtIndex == v.size())
    {
        m_gfx.fillRect(p.x - 1, p.y - k_imgOk.height - 1, k_imgOk.width + 2, k_imgOk.height + 2, 0xFFFF);
    }
    m_gfx.drawRGBA8888Bitmap(p.x, p.y - k_imgOk.height, (uint32_t*)k_imgOk.pixel_data, k_imgOk.width, k_imgOk.height);
    if (crtIndex == v.size() + 1)
    {
        m_gfx.fillRect(p.x + k_imgOk.width + 4 - 1, p.y - k_imgCancel.height - 1, k_imgCancel.width + 2, k_imgCancel.height + 2, 0xFFFF);
    }
    m_gfx.drawRGBA8888Bitmap(p.x + k_imgOk.width + 4, p.y - k_imgCancel.height, (uint32_t*)k_imgCancel.pixel_data, k_imgCancel.width, k_imgCancel.height);
}