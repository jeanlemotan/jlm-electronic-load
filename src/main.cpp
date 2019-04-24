#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include "Adafruit_ILI9341.h"
#include "DeltaBitmap.h"
#include "AiEsp32RotaryEncoder.h"
#include "Button.h"
#include "esp_spiffs.h"
#include <SPI.h>
#include <driver/rtc_io.h>
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "esp_err.h"
#include "sdmmc_cmd.h"
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
#include "icons.h"

static Adafruit_ILI9341 s_display = Adafruit_ILI9341(5, 4);
//static Adafruit_SSD1351 s_display(128, 128, &SPI, 5, 4, 2);
DeltaBitmap s_canvas(320, 240, 3, 3);
int16_t s_windowY = 0;

//w, h, cs, dc, mosi, clk, rst
//static Adafruit_SSD1351 s_display = Adafruit_SSD1351(128, 128, 5, 4, 23, 18, 2);

//

AiEsp32RotaryEncoder s_knob(34, 35, 39, -1);
XPT2046_Touchscreen s_touchscreen(27);
Settings s_settings;
Button s_button(36);

enum class SDCardStatus
{
  Unmounted,
  Mounted,
  Error
};
SDCardStatus s_sdCardStatus = SDCardStatus::Unmounted;

ValueWidget s_temperatureWidget(s_canvas, 0.f, "'C");
LabelWidget s_modeWidget(s_canvas, "");

Measurement s_measurement;

uint8_t s_selection = 0;

IRAM_ATTR void knobISR()
{
  s_knob.readEncoder_ISR();
}

// the setup function runs once when you press reset or power the board
void setup()
{
  Serial.begin(115200);
  //Wire.begin(-1, -1, 400000);
  //Wire.setClock(400000);

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

  s_display.begin(64000000);
  s_display.setRotation(1);
//  s_display.fillRect(0, 0, s_display.width(), s_display.height(), 0xFFFF);
  s_display.fillRect(0, 0, s_display.width(), s_display.height(), 0x0);

  s_touchscreen.begin();
  s_touchscreen.setRotation(1);

  s_knob.begin();
  s_knob.setup(&knobISR);
  s_knob.enable();
  s_knob.setBoundaries(-10000, 10000);
  s_knob.reset();

  s_windowY = 16; //SansSerif_bold_10.yAdvance + 3;

  s_settings.load();
  s_measurement.init();
  s_measurement.setSettings(s_settings);

  initMeasurementState();
  initCalibrationState();

  s_modeWidget.setTextColor(0x0);
  s_modeWidget.setPosition(Widget::Position{3, 0});
  s_modeWidget.setFont(&SansSerif_bold_10);

  s_temperatureWidget.setMainColor(0x0);
  s_temperatureWidget.setSuffixColor(0x0);
  s_temperatureWidget.setDecimals(1);
  s_temperatureWidget.setValueFont(&SansSerif_bold_10);
  s_temperatureWidget.setSuffixFont(&SansSerif_bold_10);

	if (s_knob.buttonState() == RotaryEncoder::ButtonState::Down)
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
  bool alreadyMounted = s_sdCardStatus != SDCardStatus::Unmounted;
  if (alreadyMounted == cardPresent)
  {
    return;
  }

  if (cardPresent)
  {
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
    slot_config.gpio_miso = GPIO_NUM_2;
    slot_config.gpio_mosi = GPIO_NUM_15;
    slot_config.gpio_sck = GPIO_NUM_14;
    slot_config.gpio_cs = GPIO_NUM_13;
    slot_config.gpio_cd = GPIO_NUM_12;

    gpio_set_pull_mode(GPIO_NUM_12, GPIO_PULLUP_ONLY);

    esp_vfs_fat_sdmmc_mount_config_t mount_config =
    {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t *card;
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
                      "Make sure SD card lines have pull-up resistors in place.",
                 esp_err_to_name(ret));
      }
      s_sdCardStatus = SDCardStatus::Error;
      return;
    }
    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);
    s_sdCardStatus = SDCardStatus::Mounted;
  }
  else
  {
    esp_vfs_fat_sdmmc_unmount();
    s_sdCardStatus = SDCardStatus::Unmounted;
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
  s_temperatureWidget.setValue(s_measurement.getTemperature());
  s_temperatureWidget.setPosition(Widget::Position{s_canvas.width() - s_temperatureWidget.getWidth() - 5, 0});
  s_temperatureWidget.render();

  if (s_touchscreen.touched())
  {
    Touchscreen::Point point = s_touchscreen.getPoint();
    //printf("\n%d, %d, %d", (int)point.x, (int)point.y, (int)point.z);
    s_canvas.fillCircle(point.x / 4096.f * 320, point.y / 4096.f * 240, point.z / 4096.f * 50 + 50, 0xFFFF);
  }

  processSDCard();
  //printf("\nXXX %d", gpio_get_level(GPIO_NUM_12));

  if (s_sdCardStatus != SDCardStatus::Unmounted)
  {
    const Image* img = s_sdCardStatus == SDCardStatus::Mounted ? &k_imgCardOk : &k_imgCardError;
    s_canvas.drawRGBA8888Bitmap(s_temperatureWidget.getPosition(Widget::Anchor::TopLeft).x - 18, 0, 
  							(const uint32_t*)img->pixel_data, img->width, img->height);
  }

  s_canvas.setFont(&SansSerif_bold_13);

  processState();

  s_canvas.blit(s_display);
}
