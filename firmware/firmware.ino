#include "ADS1115.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <Encoder.h>
#include <SPI.h>
#include "PWM.h"
#include "Utils.h"

static Adafruit_SSD1351 s_display = Adafruit_SSD1351(7, 5, 4);
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

static float s_temperatureBias = 0.f;
static float s_temperatureScale = 38.3f;
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
  return raw * s_temperatureScale + s_temperatureBias;
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
  s_display.fillRect(0, 0, 128, 128, 0x0000);

  //analogReadResolution(12);
  pinMode(A0, INPUT);
  digitalWrite(A0, HIGH);
}

// the loop function runs over and over again forever
void loop()
{
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

  static int32_t lastTP = millis();
  if (lastTP + 100 < millis())
  {
    lastTP = millis();
    char vstr[12];
    char cstr[12];
    char tstr[12];
    ftoa(vstr, s_voltage, 3);
    ftoa(cstr, s_current, 3);
    ftoa(tstr, s_temperature, 1);
    
    char buffer[128];
    sprintf(buffer, "DAC: %ld\nV: %s V\nC: %s A\nT: %s 'C", (int32_t)(dac), vstr, cstr, tstr);
    Serial.println(buffer);
    s_display.setTextSize(1.5);
    s_display.setCursor(0,0);
    s_display.setTextColor(0xF800, 0x0FF5);
    s_display.print(buffer);
  }
}
