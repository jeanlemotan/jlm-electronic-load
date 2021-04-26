#include "ValueEditWidget.h"
#include "icons.h"


ValueEditWidget::ValueEditWidget(JLMBackBuffer& gfx, const char* suffix)
    : EditWidget(gfx, nullptr, suffix)
{
    _setValue(m_value);
}
void ValueEditWidget::setRange(float min, float max)
{
    m_min = std::min(min, max);
    m_max = std::max(min, max);
    _setValue(std::max(std::min(m_value, m_max), m_min));
}
void ValueEditWidget::setDecimals(uint8_t decimals)
{
	if (m_decimals == decimals)
	{
        return;
    }

	m_decimals = decimals;
    _setValue(m_value);
}
int16_t ValueEditWidget::getWidth() const
{
    int16_t w = EditWidget::getWidth();
    return w + k_imgOk.width + k_imgCancel.width + 4*2;
}
void ValueEditWidget::setValue(float value)
{
    value = std::max(std::min(value, m_max), m_min);
    if (m_value == value)
    {
        return;
    }
    m_value = value;
    _setValue(m_value);
}
void ValueEditWidget::_setValue(float value)
{
	char str[32] = { '\0' };
	size_t offset = 0;

    float maxAbsValue = std::max(m_max, std::abs(m_min));
    {
        int usedDigits = std::max((int)std::log10(std::abs(m_value)), 0) + 1;
        int maxDigits = std::max((int)std::log10(maxAbsValue), 0) + 1;
        if (maxDigits > usedDigits)
        {
            for (int i = 0; i < maxDigits - usedDigits; i++)
            {
                str[offset++] = '0';
            }
        }
    }

	if (m_decimals == 0)
	{
		sprintf(str + offset, "%d", (int32_t)m_value);
	}
	else
	{
		dtostrf(m_value, 0, m_decimals, str + offset);
	}
    setString(str);
}
float ValueEditWidget::getValue() const
{
    return m_value;
}
EditWidget::Result ValueEditWidget::process(RotaryEncoder& knob)
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
        if (crtIndexFromBack > m_decimals) //after the decimal point, we have to substract one to account for the decimal point itself
        {
            crtIndexFromBack--;
        }
        float d = (float)pow(10.0, double(crtIndexFromBack) - double(m_decimals));
        setValue(m_value + delta * d);
    }

    return Result::None;
}

void ValueEditWidget::render()
{
    EditWidget::render();

    const std::string& v = getString();
    Position p = getPosition(Anchor::BottomLeft).move(EditWidget::getWidth() + 4, 0);
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