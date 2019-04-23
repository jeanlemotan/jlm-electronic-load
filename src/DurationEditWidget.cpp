#include "DurationEditWidget.h"

static constexpr struct {
  uint16_t  	 width;
  uint16_t  	 height;
  uint16_t  	 bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */ 
  uint8_t 	 pixel_data[16 * 13 * 4 + 1];
} k_imgOk = {
  16, 13, 4,
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000]\234]\066v\263v\265&w&\014\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "S\227S\060\201\301\201\354\211\311\211\377s\265s\274\033q\033\011\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000I\222"
  "I*w\274w\350\200\305\200\377\177\305\177\377~\304~\377h\257h\244\000\000\000\000"
  "\000\000\000\000\004g\004\011\000e\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000A\215A%n\270"
  "n\344w\301w\377v\301v\377u\300u\377s\276s\361o\264o\066\000\000\000\000+|+\022f\260"
  "f\315W\241Wp\000c\000\000\000\000\000\000\000\000\000\000\000\000\000\000\070\210\070\040e\263e\337n\275n"
  "\377m\274m\377l\274l\377j\272j\365g\262g@\000\000\000\000\040w\040\016a\257a\310o\275"
  "o\377k\272k\375M\235Mg\000^\000\000\000\000\000\000/\202/\033\\\257\\\332f\271f\377d\271"
  "d\377c\270c\377a\267a\371_\257_L\003\003\003\000\000\000\000\000S\244S\250g\272g\377f\271"
  "f\377d\271d\377_\265_\374D\230D^'}'\027S\252S\325]\265]\377\\\265\\\377Z\264"
  "Z\377Y\263Y\374W\254WY\040(\040\000\000\000\000\000\000\000\000\000\070\213\070)W\256W\350\\\265"
  "\\\377[\264[\377Y\264Y\377T\257T\373L\247L\335U\262U\377P\257P\377B\246B"
  "\377\063\237\063\375-\230-e\060F\060\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\064\213"
  "\064/N\252N\354R\260R\377P\257P\377N\257N\377D\250D\377-\233-\377\021\222\021"
  "\377\016\221\016\376\017\214\017s\007\070\007\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000/\212/\065E\246E\357@\246@\377#\225#\377\002\214\002\377\002\214\002"
  "\377\002\214\002\377\003\206\003\202\003S\003\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000$\204$<\004\204\004\363\000\205\000\377\000\205\000\377\000\205"
  "\000\377\000\201\000\220\000\\\000\001\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000t\000C\000}\000\365\000\177\000\377\000|\000\236\000"
  "`\000\002\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\023\000\000\000q\000J\000v\000\246\000c\000\004\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000",
};

static constexpr struct {
  unsigned int 	 width;
  unsigned int 	 height;
  unsigned int 	 bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */ 
  unsigned char	 pixel_data[13 * 13 * 4 + 1];
} k_imgCancel = {
  13, 13, 4,
  "\273\067\062&\302>\070f\303?\070y\275:\064\065\264\061+\011\246#\036\001\212\020\017\000"
  "\237\034\031\000\252'#\002\267\064.\023\301>\067M\304B:\226\274:\064\204\303=\066m\324"
  "KC\273\331NF\314\317F>\202\301:\063-\264-(\007\247\040\035\001\254%!\002\271\062,\024"
  "\307?\070L\322I@\242\324LC\313\305A:\244\304<\066\213\331LD\325\341PH\362\333"
  "KC\322\316@:|\300\064/)\261($\011\272/*\030\310;\064T\323E=\251\330JB\326\323"
  "H@\254\302=\066[\276\066\061C\321C<\221\335IA\332\340JC\361\331D=\315\314:\064"
  "x\276/*=\310\066\061_\324@\071\261\331E>\333\324D=\261\310>\067V\271\063.\027\264"
  ",'\015\303\066\060\067\317=\067\211\332B<\325\336C<\356\330>\067\317\317\067\061"
  "\250\325<\065\301\332@\071\340\325?\070\267\310\071\063\\\272\061+\030\255(#\003\246"
  "\037\033\001\263)%\012\301\061,\061\315\067\061\205\330;\065\327\336=\066\363\335;\065"
  "\360\334<\065\355\325:\063\303\311\064/e\273-(\033\256%!\003\244\037\033\000\226\021"
  "\017\000\245\030\026\001\262!\036\017\300($O\320\060,\272\336\067\061\365\342\071\063\375"
  "\334\066\060\356\315/*\241\274&\"\066\257\037\034\007\247\033\030\000\032\001\000\000\245\022"
  "\021\001\262\030\026\011\276\037\034.\312&\"\200\326,(\324\335\061,\364\335\061,\362"
  "\332\060+\356\323+'\300\306$\040`\270\034\031\031\254\025\023\003\241\021\017\000\261"
  "\025\023\013\277\033\031\063\314\040\036\204\326&#\322\333*&\356\326)&\322\316($"
  "\256\323)%\305\327(%\340\321$!\264\305\036\033X\267\030\026\026\252\023\021\002\272"
  "\026\024?\313\034\032\214\327\040\036\327\333#\040\361\326$\040\321\312!\037}\275"
  "\037\033B\306!\036e\321\"\040\265\324!\037\333\317\036\033\256\303\031\027R\265\024"
  "\022\026\277\026\025\211\323\032\031\323\332\035\033\362\326\036\033\325\312\034\032"
  "\201\276\032\030,\260\027\026\012\270\031\027\032\304\034\032Y\316\035\032\256\322\033"
  "\031\326\314\030\026\250\275\024\023W\276\024\023q\316\027\025\277\322\030\026\320\312"
  "\027\025\207\276\026\025\060\261\025\023\010\245\022\021\001\254\024\022\003\266\026\024\026"
  "\302\027\025Q\314\027\025\246\315\026\024\315\300\023\022\243\266\022\021*\275\022\021"
  "m\276\023\021\201\271\023\021\071\260\022\021\012\245\020\020\001\213\014\013\000\241\020"
  "\016\000\252\022\020\002\264\022\021\025\274\023\021S\277\022\021\237\267\021\020\213",
};


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
    setValue(buf);
}
Clock::duration DurationEditWidget::getDuration() const
{
    return m_duration;
}
void DurationEditWidget::process(RotaryEncoder& knob)
{
    if (!isEditing())
    {
        m_isEditingDigit = false;
        return;
    }

    const std::string& v = getValue();

    if (knob.buttonState() == RotaryEncoder::ButtonState::Released)
    {
        if (getSelectedIndex() < v.size())
        {
            m_isEditingDigit = !m_isEditingDigit;
            setSelectedBackgroundColor(m_isEditingDigit ? 0x9999 : 0xFFFF);
        }
    }

    if (!m_isEditingDigit)
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

    const std::string& v = getValue();
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