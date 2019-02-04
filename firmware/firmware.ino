#include "ADS1115.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include "AiEsp32RotaryEncoder.h"
#include "Button.h"
#include "esp_spiffs.h"
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


Adafruit_SSD1351 s_display(128, 128, &SPI, 5, 4, 2);
GFXcanvas16 s_canvas(128, 128);
//static Adafruit_SSD1351 s_display = Adafruit_SSD1351(128, 128, 5, 4, 23, 18, 2);

AiEsp32RotaryEncoder s_knob(26, 27, 25, -1);
ADS1115 s_adc;
Settings s_settings;
Button s_button(33);

uint8_t k_disableDacPin = 13;

static ValueWidget s_temperatureWidget(s_canvas, 0.f, "'C");
LabelWidget s_modeWidget(s_canvas, "");


uint8_t s_selection = 0;


IRAM_ATTR void knobISR()
{
  s_knob.readEncoder_ISR();
}

// the setup function runs once when you press reset or power the board
void setup() 
{
  Serial.begin(57600);

  esp_log_level_set("*", ESP_LOG_DEBUG);

  esp_vfs_spiffs_conf_t conf = 
  {
    .base_path = "/spiffs",
    .partition_label = NULL,
    .max_files = 5,
    .format_if_mount_failed = true
  };

  // Use settings defined above to initialize and mount SPIFFS filesystem.
  // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
  esp_err_t ret = esp_vfs_spiffs_register(&conf);
  if (ret != ESP_OK) 
  {
    if (ret == ESP_FAIL) 
    {
      ESP_LOGE(TAG, "Failed to mount or format filesystem");
    } 
    else if (ret == ESP_ERR_NOT_FOUND) 
    {
      ESP_LOGE(TAG, "Failed to find SPIFFS partition");
    } 
    else 
    {
      ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
    }
    return;
  }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info(NULL, &total, &used);
  if (ret != ESP_OK) 
  {
    ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
  } 
  else 
  {
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
  }

  //pwm1Configure(PWM1_62k); //DAC PWM
  //pwm4Configure(PWM4_23k); //POWER PWM
  //pwm4Set6(128);
  pwmInit();

  s_adc.begin();
  s_adc.set_data_rate(ADS1115_DATA_RATE_8_SPS);
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

  pinMode(k_disableDacPin, OUTPUT); 
  digitalWrite(k_disableDacPin, 1);

  s_knob.begin();
  s_knob.setup(&knobISR);
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

  loadSettings(s_settings);
  //saveSettings(s_settings);

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
  s_canvas.fillScreen(0x0);
  s_canvas.fillRect(0, 0, s_canvas.width(), 9, 0xFFFF);

//Temperature
  s_temperatureWidget.setValue(getTemperature());
  s_temperatureWidget.setPosition(s_canvas.width() - s_temperatureWidget.getWidth(), 1);
  s_temperatureWidget.update();

  processState();

  s_display.drawRGBBitmap(0, 0, s_canvas.getBuffer(), s_canvas.width(), s_canvas.height());
}
