#include "PWM.h"
#include <driver/ledc.h>

void pwmInit()
{
  {
    ledc_timer_config_t config;
    config.speed_mode = LEDC_HIGH_SPEED_MODE;
    config.duty_resolution = LEDC_TIMER_16_BIT;
    config.timer_num = LEDC_TIMER_1;
    config.freq_hz = 1000;
    ESP_ERROR_CHECK(ledc_timer_config(&config));
  }
  {
    ledc_channel_config_t config;
    config.gpio_num = 12;
    config.speed_mode = LEDC_HIGH_SPEED_MODE;
    config.channel = LEDC_CHANNEL_0;
    config.intr_type = LEDC_INTR_DISABLE;
    config.timer_sel = LEDC_TIMER_1;
    config.duty = 0;
    config.hpoint = 0xFFFF;
    ESP_ERROR_CHECK(ledc_channel_config(&config));
  }

  {
    ledc_timer_config_t config;
    config.speed_mode = LEDC_HIGH_SPEED_MODE;
    config.duty_resolution = LEDC_TIMER_2_BIT;
    config.timer_num = LEDC_TIMER_3;
    config.freq_hz = 10 * 1000;
    ESP_ERROR_CHECK(ledc_timer_config(&config));
  }
  {
    ledc_channel_config_t config;
    config.gpio_num = 14;
    config.speed_mode = LEDC_HIGH_SPEED_MODE;
    config.channel = LEDC_CHANNEL_1;
    config.intr_type = LEDC_INTR_DISABLE;
    config.timer_sel = LEDC_TIMER_3;
    config.duty = 1;
    //config.hpoint = 10;
    ESP_ERROR_CHECK(ledc_channel_config(&config));
  }
  ESP_ERROR_CHECK(ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, 2, 3));
}


void setDACPWM(float dac)
{
  uint32_t duty = uint32_t(dac * 65535.f);
  ESP_ERROR_CHECK(ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty, 0xFFFF));
}
