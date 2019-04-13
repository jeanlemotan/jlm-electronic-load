#include "ADS1115.h"
#include "DAC8571.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include "Adafruit_ILI9341.h"
#include "DeltaBitmap.h"
#include "AiEsp32RotaryEncoder.h"
#include "Button.h"
#include "esp_spiffs.h"
#include <SPI.h>
#include <driver/adc.h>
#include <driver/rtc_io.h>
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "esp_err.h"
#include "sdmmc_cmd.h"
#include "PWM.h"
#include "Settings.h"
#include "ValueWidget.h"
#include "LabelWidget.h"
#include "CalibrationState.h"
#include "MeasurementState.h"
#include "Measurement.h"
#include "State.h"
#include "XPT2046.h"
#include "Fonts/SansSerif_plain_10.h"
#include "Fonts/SansSerif_bold_10.h"
#include "Fonts/SansSerif_plain_12.h"
#include "Fonts/SansSerif_bold_12.h"
#include "Fonts/SansSerif_plain_13.h"
#include "Fonts/SansSerif_bold_13.h"

static Adafruit_ILI9341 s_display = Adafruit_ILI9341(5, 4);
//static Adafruit_SSD1351 s_display(128, 128, &SPI, 5, 4, 2);
DeltaBitmap s_canvas(320, 240, 4, 4);
int16_t s_windowY = 0;

//w, h, cs, dc, mosi, clk, rst
//static Adafruit_SSD1351 s_display = Adafruit_SSD1351(128, 128, 5, 4, 23, 18, 2);

//

AiEsp32RotaryEncoder s_knob(34, 35, 39, -1);
ADS1115 s_adc;
DAC8571 s_dac;
XPT2046_Touchscreen s_touchscreen(27);
Settings s_settings;
Button s_button(36);

uint8_t k_4WirePin = 16;
bool s_sdCardMounted = false;

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
  Serial.begin(115200);

  esp_log_level_set("*", ESP_LOG_DEBUG);

  {
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
  }

  //pwm1Configure(PWM1_62k); //DAC PWM
  //pwm4Configure(PWM4_23k); //POWER PWM
  //pwm4Set6(128);
  pwmInit();

  s_adc.begin();
  s_adc.set_data_rate(ADS1115_DATA_RATE_8_SPS);
  s_adc.set_mode(ADS1115_MODE_SINGLE_SHOT);
  s_adc.set_mux(ADS1115_MUX_GND_AIN0); //switch to voltage pair
  s_adc.set_pga(ADS1115_PGA_SIXTEEN);
  if (s_adc.trigger_sample() != 0)
  {
      Serial.println("adc read trigger failed (ads1115 not connected?)");
  }

  s_dac.begin();
  s_dac.write(0);

//  SPI.begin();
//  SPI.setDataMode(SPI_MODE3);
  s_display.begin(64000000);
  s_display.setRotation(1);
  s_display.fillRect(0, 0, s_display.width(), s_display.height(), 0xFFFF);
  s_display.fillRect(0, 0, s_display.width(), s_display.height(), 0x0);

  {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_6);
    //ESP_ERROR_CHECK(gpio_pullup_en(GPIO_NUM_32));
  }

  pinMode(k_4WirePin, OUTPUT); 
  //digitalWrite(k_4WirePin, 1);

  s_touchscreen.begin();
  s_touchscreen.setRotation(1);

  s_knob.begin();
  s_knob.setup(&knobISR);
  s_knob.enable();
  s_knob.setBoundaries(-10000, 10000);
  s_knob.reset();

  s_windowY = SansSerif_bold_10.yAdvance + 3;  

  initMeasurementState();
  initCalibrationState();

  loadSettings(s_settings);
  //saveSettings(s_settings);

  s_modeWidget.setTextScale(1);
  s_modeWidget.setTextColor(0x0);
  s_modeWidget.setPosition(Widget::Position{3, 0});
  s_modeWidget.setFont(&SansSerif_bold_10);

  s_temperatureWidget.setTextScale(1);
  s_temperatureWidget.setValueColor(0x0);
  s_temperatureWidget.setSuffixColor(0x0);
  s_temperatureWidget.setDecimals(1);
  s_temperatureWidget.setValueFont(&SansSerif_bold_10);
  s_temperatureWidget.setSuffixFont(&SansSerif_bold_10);

  if (1 || s_knob.currentButtonState() == BUT_DOWN)
  {
    setState(State::Calibration);
  }
  else
  {
    setState(State::Measurement);
  }

}

void processSDCard()
{
  bool cardPresent = gpio_get_level(GPIO_NUM_12) == 0;
  if (s_sdCardMounted == cardPresent)
  {
      return;
  }

  if (cardPresent)
  {
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
    slot_config.gpio_miso = GPIO_NUM_2;
    slot_config.gpio_mosi = GPIO_NUM_15;
    slot_config.gpio_sck  = GPIO_NUM_14;
    slot_config.gpio_cs   = GPIO_NUM_13;
    slot_config.gpio_cd   = GPIO_NUM_12;

    gpio_set_pull_mode(GPIO_NUM_12, GPIO_PULLUP_ONLY);

    esp_vfs_fat_sdmmc_mount_config_t mount_config = 
    {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) 
    {
        if (ret == ESP_FAIL) 
        {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                "If you want the card to be formatted, set format_if_mount_failed = true.");
        } 
        else 
        {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }
    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);
    s_sdCardMounted = true;
  }
  else
  {
    esp_vfs_fat_sdmmc_unmount();    
    s_sdCardMounted = false;
  }
}


// the loop function runs over and over again forever
void loop()
{
  static uint32_t lastTP = millis();
  printf("\nD: %lu", millis() - lastTP);
  lastTP = millis();

  s_canvas.fillScreen(0);

  s_canvas.setFont(&SansSerif_bold_10);
  s_canvas.fillRect(0, 0, s_canvas.width(), s_windowY, 0xFFFF);

  //Temperature
  s_temperatureWidget.setValue(getTemperature());
  s_temperatureWidget.setPosition(Widget::Position{s_canvas.width() - s_temperatureWidget.getWidth() - 5, 0});
  s_temperatureWidget.render();

  if (s_touchscreen.touched())
  {
    TS_Point point = s_touchscreen.getPoint();
    //printf("\n%d, %d, %d", (int)point.x, (int)point.y, (int)point.z);
    s_canvas.fillCircle(point.x / 4096.f * 320, point.y / 4096.f * 240, point.z / 4096.f * 50 + 50, 0xFFFF);
  }

  processSDCard();
  //printf("\nXXX %d", gpio_get_level(GPIO_NUM_12));

  uint8_t bmp[] = 
  {
    0x01, 0xfc, 0x03, 0x54, 0x07, 0x54, 0x0f, 0x54, 0x1f, 0x54, 0x3f, 0x54, 0x3f, 0xfc, 0x3f, 0xfc,
    0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc
  };
  if (s_sdCardMounted)
  {
    s_canvas.drawBitmap(s_canvas.width() - 18, s_canvas.height() - 18, bmp, 16, 16, 0xFFFF);
  }

  s_canvas.setFont(&SansSerif_bold_13);

  processState();

  s_canvas.blit(s_display);
}
