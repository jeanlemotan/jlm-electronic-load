#include "ADS1115.h"
//#include <Adafruit_GFX.h>
//#include <Fonts/FreeMonoBold9pt7b.h>
//#include <Fonts/FreeMonoBold12pt7b.h>
//#include <Adafruit_SSD1351.h>
//#include "Ucglib.h"
#include <FTOLED.h>
#include <fonts/Droid_Sans_12.h>
#include <Encoder.h>
#include <SPI.h>
#include "PWM.h"
#include "Utils.h"

//static Adafruit_SSD1351 s_display = Adafruit_SSD1351(7, 5, 4);
Ucglib_SSD1351_18x128x128_FT_HWSPI s_display(5, 7, 4);
static Encoder s_knob(1, 8);
static ADS1115 s_adc;

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

int drawQuantityLine(int y, uint8_t flags, float q, const char* unit)
{
  s_display.setColor(1, 0, 0, 0);
  if (flags & (uint8_t)LineFlags::Invalid)
  {
    s_display.setColor(0, 255, 0, 0);
  }
  else
  {
    s_display.setColor(0, 255, 255, 255);
  }
  s_display.setFont(ucg_font_helvB14_hr);
  int h = s_display.getFontAscent() - s_display.getFontDescent();
  s_display.setPrintPos(0, y + s_display.getFontAscent());
  s_display.print(q, 3);
  s_display.setFont(ucg_font_helvB08_hr);
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

  static int selection = 0;
  selection += s_knob.read();

  s_display.setFontMode(UCG_FONT_MODE_SOLID);

  int y = 0;
  int h = 0;

  //Mode
  y = 1;
  s_display.setColor(1, 255, 255, 255);
  s_display.setColor(0, 0, 0, 0);
  s_display.setFont(ucg_font_helvB08_hr);
  h = s_display.getFontAscent() - s_display.getFontDescent();
  s_display.setPrintPos(1, y + s_display.getFontAscent());
  s_display.print("CC Mode");

  //Temperature
  static uint32_t lastTemperatureDisplayTP = millis();
  if (millis() > lastTemperatureDisplayTP + 1000)
  {
    lastTemperatureDisplayTP = millis();
    char buffer[128];
    sprintf(buffer, "%d'C", (int)s_temperature);
    s_display.setFont(ucg_font_helvB08_hr);
    int w = s_display.getStrWidth(buffer);
    s_display.setPrintPos(s_display.getWidth() - w, y + s_display.getFontAscent());
    s_display.print(buffer);
  }
  y += h + 10;

  s_display.setColor(0, 255, 255, 255);
  s_display.setColor(1, 0, 0, 0);

  y += drawQuantityLine(y, 0, s_voltage, "V") + 2;
  y += drawQuantityLine(y, 0, s_current, "A") + 2;
  y += drawQuantityLine(y, 0, s_voltage * s_current, "W") + 2;

  { 
    s_display.setColor(1, 0, 0, 0);
    if (selection == 3)
    {
      s_display.setColor(1, 64, 64, 64);
    }
    s_display.setColor(0, 255, 255, 255);
    s_display.setFont(ucg_font_helvB24_hr);
    h = s_display.getFontAscent() - s_display.getFontDescent();
    s_display.setPrintPos(0, y + s_display.getFontAscent());
    s_display.print(s_current, 3);
    s_display.setFont(ucg_font_helvB08_hr);
    s_display.print("A");
    y += h + 2;
  }
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

  s_display.begin(UCG_FONT_MODE_TRANSPARENT);
  s_display.setColor(0, 0, 0, 0);
  s_display.drawBox(0, 0, s_display.getWidth(), s_display.getHeight());

  s_display.setFontMode(UCG_FONT_MODE_TRANSPARENT);
  //title line
  {
    s_display.setFont(ucg_font_helvB08_hr);
    int h = s_display.getFontAscent() - s_display.getFontDescent();
    s_display.setColor(0, 255, 255, 255);
    s_display.drawBox(0, 0, s_display.getWidth(), h + 1);
  }

  pinMode(A0, INPUT_PULLUP);
}

// the loop function runs over and over again forever
void loop()
{
  processNormalSection();
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

  int32_t knob = s_knob.read();
}
