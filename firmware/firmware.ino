#include "ADS1115.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include "AiEsp32RotaryEncoder.h"
#include <SPI.h>
#include <driver/adc.h>
#include <driver/rtc_io.h>
#include "PWM.h"
#include "ValueWidget.h"
#include "LabelWidget.h"
#include "Settings.h"

static Adafruit_SSD1351 s_display = Adafruit_SSD1351(128, 128, &SPI, 5, 4, 2);
//static Adafruit_SSD1351 s_display = Adafruit_SSD1351(128, 128, 5, 4, 23, 18, 2);

static AiEsp32RotaryEncoder s_knob(26, 27, -1, -1);
static ADS1115 s_adc;

static LabelWidget s_modeWidget(s_display, "");
static ValueWidget s_temperatureWidget(s_display, 0.f, "'C");
static ValueWidget s_voltageWidget(s_display, 0.f, "V");
static ValueWidget s_currentWidget(s_display, 0.f, "A");
static ValueWidget s_powerWidget(s_display, 0.f, "W");
static ValueWidget s_trackedWidget(s_display, 0.f, "A");
static ValueWidget s_dacWidget(s_display, 0.f, "");

static float s_currentRaw = 0.f;
static float s_voltageRaw = 0.f;
static float s_current = 0.f;
static float s_voltage = 0.f;
static bool s_loadOn = false;

Settings s_settings;
static ads1115_pga s_rangePgas[k_rangeCount] = { ADS1115_PGA_SIXTEEN, ADS1115_PGA_EIGHT, ADS1115_PGA_FOUR, ADS1115_PGA_TWO, ADS1115_PGA_ONE };

static uint8_t s_currentRange = 4;
static uint8_t s_voltageRange = 2;

enum class ADCMux
{
  Current,
  Voltage
};

static ADCMux s_adcMux = ADCMux::Voltage;

static float s_temperatureRaw = 0.f;
static float s_temperature = 0.f;

void getCurrentBiasScale(float& bias, float& scale)
{
  bias = s_settings.currentRangeBiases[s_currentRange];
  scale = s_settings.currentRangeScales[s_currentRange];
}
float computeCurrent(float raw)
{
  float bias, scale;
  getCurrentBiasScale(bias, scale);
  return (raw + bias) * scale;
}

void getVoltageBiasScale(float& bias, float& scale)
{
  bias = s_settings.voltageRangeBiases[s_voltageRange];
  scale = s_settings.voltageRangeScales[s_voltageRange];
}
float computeVoltage(float raw)
{
  float bias, scale;
  getVoltageBiasScale(bias, scale);
  return (raw + bias) * scale;
}

float computeTemperature(float raw)
{
  return (raw + s_settings.temperatureBias) * s_settings.temperatureScale;
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
      s_voltage = (s_voltage * 90.f + computeVoltage(s_voltageRaw) * 10.f) / 100.f;
      
      s_adc.set_mux(ADS1115_MUX_DIFF_AIN2_AIN3); //switch to current pair
      s_adc.set_pga(s_rangePgas[s_currentRange]);
      s_adc.trigger_sample();
      s_adcMux = ADCMux::Current;
    }
    else
    {
      s_currentRaw = val;
      s_current = (s_current * 90.f + computeCurrent(s_currentRaw) * 10.f) / 100.f;
      
      s_adc.set_mux(ADS1115_MUX_DIFF_AIN0_AIN1); //switch to voltage pair
      s_adc.set_pga(s_rangePgas[s_voltageRange]);
      s_adc.trigger_sample();
      s_adcMux = ADCMux::Voltage;
    }
  }

  s_temperatureRaw = 1.f - (adc1_get_raw(ADC1_CHANNEL_4) / 4096.f);
  s_temperature = (s_temperature * 99.f + computeTemperature(s_temperatureRaw) * 1.f) / 100.f;
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

uint8_t s_selection = 0;

void processNormalSection()
{
  readAdcs();

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

  static int32_t dac = 1100;
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

  int knobDelta = s_knob.encoderChanged();
  if (knobDelta != 0)
  {
    static uint32_t lastTP = millis();
    float dt = (millis() - lastTP) / 1000.f;
    lastTP = millis();

    int32_t d = (knobDelta / dt) * 0.05f;
    if (d == 0)
    {
      d = knobDelta < 0 ? -1 : 1;
    }
    dac += d;
    dac = max(dac, 0);
  }

  float dacf = dac / float(k_dacPrecision);
  setDACPWM(dacf);
  s_dacWidget.setValue(dacf);

  s_dacWidget.update();
}

// the setup function runs once when you press reset or power the board
void setup() 
{
  Serial.begin(57600);

  //pwm1Configure(PWM1_62k); //DAC PWM
  //pwm4Configure(PWM4_23k); //POWER PWM
  //pwm4Set6(128);
  pwmInit();

  s_adc.begin();
  s_adc.set_data_rate(ADS1115_DATA_RATE_250_SPS);
  s_adc.set_mode(ADS1115_MODE_SINGLE_SHOT);
  s_adc.set_mux(ADS1115_MUX_DIFF_AIN0_AIN1); //switch to voltage pair
  s_adc.set_pga(s_rangePgas[s_voltageRange]);
  if (s_adc.trigger_sample() != 0)
  {
      Serial.println("adc read trigger failed (ads1115 not connected?)");
  }

//  SPI.begin();
//  SPI.setDataMode(SPI_MODE3);
  s_display.begin(16000000);
  s_display.fillRect(0, 0, s_display.width(), s_display.height(), 0xFFFF);
  delay(100);
  s_display.fillRect(0, 0, s_display.width(), s_display.height(), 0x0);

  //title line
  {
    //s_display.setFont(ucg_font_helvB08_hr);
    int h = 8;
    s_display.fillRect(0, 0, s_display.width(), h + 1, 0xFFFF);
  }

  {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_11);
    ESP_ERROR_CHECK(gpio_pullup_en(GPIO_NUM_32));
  }

  s_knob.begin();
  s_knob.setup([]{s_knob.readEncoder_ISR();});
  s_knob.enable();
  s_knob.setBoundaries(-10000, 10000);
  s_knob.reset();

  {
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_26, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_pullup_en(GPIO_NUM_26));
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_27, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_pullup_en(GPIO_NUM_27));
  }

/*
  int int0 = digitalPinToInterrupt(12);
  int int1 = digitalPinToInterrupt(13);
  Serial.print(int0);
  Serial.print(", ");
  Serial.println(int1);
  attachInterrupt(int0, &knobIsr, CHANGE);
  attachInterrupt(int1, &knobIsr, CHANGE);
  */

  s_modeWidget.setTextScale(1);
  s_modeWidget.setTextColor(0x0);
  s_modeWidget.setBackgroundColor(0xFFFF);
  s_modeWidget.setPosition(1, 1);

  s_temperatureWidget.setTextScale(1);
  s_temperatureWidget.setTextColor(0x0);
  s_temperatureWidget.setBackgroundColor(0xFFFF);
  s_temperatureWidget.setDecimals(1);

  s_voltageWidget.setTextScale(1);
  s_voltageWidget.setDecimals(6);
  s_voltageWidget.setPosition(0, 30);

  s_currentWidget.setTextScale(1);
  s_currentWidget.setDecimals(6);
  s_currentWidget.setPosition(s_voltageWidget.getX(), s_voltageWidget.getY() + s_voltageWidget.getHeight() + 2);

  s_powerWidget.setTextScale(2);
  s_powerWidget.setPosition(s_currentWidget.getX(), s_currentWidget.getY() + s_currentWidget.getHeight() + 2);

  s_trackedWidget.setTextScale(3);
  s_trackedWidget.setPosition(s_powerWidget.getX(), s_powerWidget.getY() + s_powerWidget.getHeight() + 2);

  s_dacWidget.setTextScale(2);
  s_dacWidget.setDecimals(5);
  s_dacWidget.setPosition(s_trackedWidget.getX(), s_trackedWidget.getY() + s_trackedWidget.getHeight() + 2);
}


// the loop function runs over and over again forever
void loop()
{
  //uint32_t start = millis();
  //s_display.fillRect(0, 0, s_display.width(), s_display.height(), 0xFFFF);
  //s_display.fillRect(0, 0, s_display.width(), s_display.height(), 0x0);

  processNormalSection();
  //Serial.println(millis() - start);
}
