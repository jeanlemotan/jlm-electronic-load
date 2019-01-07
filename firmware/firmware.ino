#include "ADS1115.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include "AiEsp32RotaryEncoder.h"
#include <SPI.h>
#include <driver/adc.h>
#include <driver/rtc_io.h>
#include "PWM.h"
#include "Settings.h"
#include "ValueWidget.h"
#include "LabelWidget.h"
#include "CalibrationState.h"
#include "MeasurementState.h"
#include "State.h"


Adafruit_SSD1351 s_display = Adafruit_SSD1351(128, 128, &SPI, 5, 4, 2);
//static Adafruit_SSD1351 s_display = Adafruit_SSD1351(128, 128, 5, 4, 23, 18, 2);

AiEsp32RotaryEncoder s_knob(26, 27, 25, -1);
ADS1115 s_adc;
Settings s_settings;

static ValueWidget s_temperatureWidget(s_display, 0.f, "'C");
LabelWidget s_modeWidget(s_display, "");


uint8_t s_selection = 0;


// the setup function runs once when you press reset or power the board
void setup() 
{
  Serial.begin(57600);

  //pwm1Configure(PWM1_62k); //DAC PWM
  //pwm4Configure(PWM4_23k); //POWER PWM
  //pwm4Set6(128);
  pwmInit();

  s_adc.begin();
  s_adc.set_data_rate(ADS1115_DATA_RATE_860_SPS);
  s_adc.set_mode(ADS1115_MODE_SINGLE_SHOT);
  s_adc.set_mux(ADS1115_MUX_DIFF_AIN0_AIN1); //switch to voltage pair
  s_adc.set_pga(ADS1115_PGA_SIXTEEN);
  if (s_adc.trigger_sample() != 0)
  {
      Serial.println("adc read trigger failed (ads1115 not connected?)");
  }

//  SPI.begin();
//  SPI.setDataMode(SPI_MODE3);
  s_display.begin(16000000);
  s_display.fillRect(0, 0, s_display.width(), s_display.height(), 0xFFFF);
  s_display.fillRect(0, 0, s_display.width(), s_display.height(), 0x0);

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

  initMeasurementState();
  initCalibrationState();

  s_modeWidget.setTextScale(1);
  s_modeWidget.setTextColor(0x0);
  s_modeWidget.setBackgroundColor(0xFFFF);
  s_modeWidget.setPosition(1, 1);

  s_temperatureWidget.setTextScale(1);
  s_temperatureWidget.setTextColor(0x0);
  s_temperatureWidget.setBackgroundColor(0xFFFF);
  s_temperatureWidget.setDecimals(1);

  if (1 || s_knob.currentButtonState() == BUT_DOWN)
  {
    setState(State::Calibration);
  }
  else
  {
    setState(State::Measurement);
  }
}



// the loop function runs over and over again forever
void loop()
{
//Temperature
  s_temperatureWidget.setValue(getTemperature());
  s_temperatureWidget.setPosition(s_display.width() - s_temperatureWidget.getWidth(), 1);
  s_temperatureWidget.update();

  processState();
}
