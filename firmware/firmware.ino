#include "ADS1115.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <RotaryEncoder.h>
#include <SPI.h>
#include "PWM.h"
#include "Utils.h"
#include "ValueWidget.h"
#include "LabelWidget.h"

static Adafruit_SSD1351 s_display = Adafruit_SSD1351(7, 5, 4);
static RotaryEncoder s_knob(0, 1);
static ADS1115 s_adc;

static LabelWidget s_modeWidget(s_display, "");
static ValueWidget s_temperatureWidget(s_display, 0.f, "'C");
static ValueWidget s_voltageWidget(s_display, 0.f, "V");
static ValueWidget s_currentWidget(s_display, 0.f, "A");
static ValueWidget s_powerWidget(s_display, 0.f, "W");
static ValueWidget s_trackedWidget(s_display, 0.f, "A");

static float s_currentRaw = 0.f;
static float s_voltageRaw = 0.f;
static float s_current = 0.f;
static float s_voltage = 0.f;
static bool s_loadOn = false;

static constexpr uint8_t k_rangeCount = 5;
static uint16_t s_rangePgas[k_rangeCount] = { ADS1115_PGA_SIXTEEN, ADS1115_PGA_EIGHT, ADS1115_PGA_FOUR, ADS1115_PGA_TWO, ADS1115_PGA_ONE };
static float s_currentRangeBiases[k_rangeCount] = { 0.f, 0.f, 0.f, 0.f, 0.f };
static float s_currentRangeScales[k_rangeCount] = { 1.f, 1.f, 1.f, 1.f, 1.f };
static float s_voltageRangeBiases[k_rangeCount] = { 0.f, 0.f, 0.f, 0.f, 0.f };
static float s_voltageRangeScales[k_rangeCount] = { 1.f, 1.f, 1.f, 1.f, 1.f };

static uint8_t s_currentRange = 0;
static uint8_t s_voltageRange = 0;

enum class ADCMux
{
  Current,
  Voltage
};

static ADCMux s_adcMux = ADCMux::Voltage;

static float s_temperatureBias = -0.54f;
static float s_temperatureScale = 126.8f;
static float s_temperatureRaw = 0.f;
static float s_temperature = 0.f;

void getCurrentBiasScale(float& bias, float& scale)
{
  bias = s_currentRangeBiases[s_currentRange];
  scale = s_currentRangeScales[s_currentRange];
}
float computeCurrent(float raw)
{
  float bias, scale;
  getCurrentBiasScale(bias, scale);
  return raw * scale + bias;
}

void getVoltageBiasScale(float& bias, float& scale)
{
  bias = s_voltageRangeBiases[s_voltageRange];
  scale = s_voltageRangeScales[s_voltageRange];
}
float computeVoltage(float raw)
{
  float bias, scale;
  getVoltageBiasScale(bias, scale);
  return raw * scale + bias;
}

float computeTemperature(float raw)
{
  return (raw + s_temperatureBias) * s_temperatureScale;
}

void readAdcs()
{
  if (!s_adc.is_sample_in_progress())
  {
    float val = s_adc.read_sample_float();
    if (val < 0)
    {
      val = 0;
    }
    if (s_adcMux == ADCMux::Voltage)
    {
      s_voltageRaw = val;
      s_voltage = computeVoltage(s_voltageRaw);
      
      s_adc.set_mux(ADS1115_MUX_DIFF_AIN2_AIN3); //switch to current pair
      s_adc.set_pga(s_rangePgas[s_currentRange]);
      s_adc.trigger_sample();
    }
    else
    {
      s_currentRaw = val;
      s_current = computeCurrent(s_currentRaw);
      
      s_adc.set_mux(ADS1115_MUX_DIFF_AIN0_AIN1); //switch to voltage pair
      s_adc.set_pga(s_rangePgas[s_voltageRange]);
      s_adc.trigger_sample();
    }
  }

  s_temperatureRaw = 1.f - (analogRead(A0) / 1024.f);
  s_temperature = computeTemperature(s_temperatureRaw);
}

bool isVoltageValid()
{
  return s_voltage > -0.1f;
}


enum class Measurement
{
  ConstantCurrent,
  ConstantPower
};

enum class LineFlags
{
  Invalid = 1 << 0,
  Selected = 1 << 1,
  Edited = 1 << 2,
};

int getTextWidth(const char* str)
{
  int x, y, w, h;
  s_display.getTextBounds(str, 0, 0, &x, &y, &w, &h);
  return w;
}
int getTextHeight(const char* str)
{
  int x, y, w, h;
  s_display.getTextBounds(str, 0, 0, &x, &y, &w, &h);
  return h;
}

uint8_t s_selection = 0;

int drawQuantityLine(uint8_t index, int y, uint8_t flags, float q, const char* unit)
{
  uint16_t fg = 0xFFFF, bg = 0x0;
  if (flags & (uint8_t)LineFlags::Invalid)
  {
    fg = 0xF000;
  }
  if (s_selection == index)
  {
    bg = 0xAAAA;
  }
  s_display.setTextColor(fg, bg);
//  s_display.setFont(ucg_font_helvB14_hr);
  s_display.setTextSize(2);
  int h = 16;//s_display.getFontAscent() - s_display.getFontDescent();
  s_display.setCursor(0, y);
  if (q >= 0.f)
  {
    s_display.print(" ");
  }
  s_display.print(q, 3);
  s_display.setTextSize(1);
  s_display.setCursor(s_display.getCursorX(), y + 7);
//  s_display.setFont(ucg_font_helvB08_hr);
  s_display.print(unit);
  return h;
}

void processNormalSection()
{
  readAdcs();

  static int xxx = 0;
  xxx--;
  s_voltage = xxx / 100.f;
  s_current = xxx / 1000.f;
  //delay(500);

  s_selection = s_knob.getPosition() & 3;

  //Mode
  s_modeWidget.setValue("CC Mode");
  s_modeWidget.update();

  //Temperature
  s_temperatureWidget.setValue(s_temperature);
  s_temperatureWidget.setPosition(s_display.width() - s_temperatureWidget.getWidth(), 1);
  s_temperatureWidget.update();

  s_voltageWidget.setTextColor(isVoltageValid() ? 0xFFFF : 0xF000);
  s_voltageWidget.setValue(s_voltage);
  s_voltageWidget.update();

  s_currentWidget.setValue(s_current);
  s_currentWidget.update();

  s_powerWidget.setValue(s_voltage * s_current);
  s_powerWidget.update();

  s_trackedWidget.setValue(s_current);
  s_trackedWidget.update();
}

void int2Isr()
{
  s_knob.tick();
}
void int3Isr()
{
  s_knob.tick();
}

// the setup function runs once when you press reset or power the board
void setup() 
{
  Serial.begin(57600);

  pwm1Configure(PWM1_62k); //DAC PWM
  pwm4Configure(PWM4_23k); //POWER PWM
  pwm4Set6(128);

  s_adc.begin();
  s_adc.set_data_rate(ADS1115_DATA_RATE_250_SPS);
  s_adc.set_mode(ADS1115_MODE_SINGLE_SHOT);
  s_adc.set_mux(ADS1115_MUX_DIFF_AIN0_AIN1); //switch to voltage pair
  s_adc.set_pga(s_rangePgas[s_voltageRange]);
  if (s_adc.trigger_sample() != 0)
  {
      Serial.println("adc read trigger failed (ads1115 not connected?)");
  }

  s_display.begin();
  s_display.fillRect(0, 0, s_display.width(), s_display.height(), 0x0);

  //title line
  {
    //s_display.setFont(ucg_font_helvB08_hr);
    int h = 8;
    s_display.fillRect(0, 0, s_display.width(), h + 1, 0xFFFF);
  }

  pinMode(A0, INPUT_PULLUP);

  int int0 = digitalPinToInterrupt(0);
  int int1 = digitalPinToInterrupt(1);
  Serial.print(int0);
  Serial.print(", ");
  Serial.println(int1);
  attachInterrupt(int0, &int2Isr, CHANGE);
  attachInterrupt(int1, &int3Isr, CHANGE);

  s_modeWidget.setTextScale(1);
  s_modeWidget.setTextColor(0x0);
  s_modeWidget.setBackgroundColor(0xFFFF);
  s_modeWidget.setPosition(1, 1);

  s_temperatureWidget.setTextScale(1);
  s_temperatureWidget.setTextColor(0x0);
  s_temperatureWidget.setBackgroundColor(0xFFFF);
  s_temperatureWidget.setDecimals(1);

  s_voltageWidget.setTextScale(2);
  s_voltageWidget.setPosition(0, 30);

  s_currentWidget.setTextScale(2);
  s_currentWidget.setPosition(s_voltageWidget.getX(), s_voltageWidget.getY() + s_voltageWidget.getHeight() + 2);

  s_powerWidget.setTextScale(2);
  s_powerWidget.setPosition(s_currentWidget.getX(), s_currentWidget.getY() + s_currentWidget.getHeight() + 2);

  s_trackedWidget.setTextScale(3);
  s_trackedWidget.setPosition(s_powerWidget.getX(), s_powerWidget.getY() + s_powerWidget.getHeight() + 2);
}


// the loop function runs over and over again forever
void loop()
{
  uint32_t start = millis();
  //s_display.fillRect(0, 0, s_display.width(), s_display.height(), 0xFFFF);

  processNormalSection();
  Serial.println(millis() - start);
  return;
  
  readAdcs();
  
  static uint16_t dac = 0;
  if (Serial.available() > 0) 
  {
    if (Serial.peek() == 'r' || Serial.peek() == 'R')
    {
      static bool highRange = false;
      highRange = !highRange;
      digitalWrite(1, highRange ? LOW : HIGH); //high range is with the relay low
      if (highRange)
      {
        s_adc.set_mux(ADS1115_MUX_GND_AIN0);
      }
      else
      {
        s_adc.set_mux(ADS1115_MUX_GND_AIN1);
      }
      s_adc.trigger_sample();
    }
    else
    {
      dac = Serial.parseInt();
      //if (Serial.read() == '\n')
      {
          Serial.print("DAC = ");
          Serial.println(dac);
      }
    }
    while (Serial.available()) 
    {
      Serial.read();
    }
  }
  //digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  //delay(3);                       // wait for a second

  static float adc = 0;
  {
    float val = s_adc.read_sample_float();
    if (val < 0)
    {
      val = 0;
    }
    adc = (adc * 99.f + val) / 100.f;
  }

  uint16_t step = 1;
  /*
  if (abs(adc - target) > 0.0002f)
  {
    step = 10;
  }
  else if (abs(adc - target) > 0.0001f)
  {
    step = 5;
  }
  else if (abs(adc - target) > 0.00005f)
  {
    step = 2;
  }
  else
  {
    step = 1;
  }*/

  pwm1Set9(dac);

  int32_t knob = s_knob.getPosition();
}
