#include <Arduino.h>
//#include <Wire.h>
#include <driver/i2c.h>
#include "ADS1115.h"

#define SAMPLE_BIT (0x8000)
#define WRITE_BIT                          I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT                           I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN                       0x1              /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS                      0x0              /*!< I2C master will not check ack from slave */

enum ads1115_register 
{
  ADS1115_REGISTER_CONVERSION = 0,
  ADS1115_REGISTER_CONFIG = 1,
  ADS1115_REGISTER_LOW_THRESH = 2,
  ADS1115_REGISTER_HIGH_THRESH = 3,
};

#define FACTOR 32768.0
static float ranges[] = { 6.144, 4.096, 2.048, 1.024, 0.512, 0.256 };

ADS1115::ADS1115(uint8_t address)
{
  m_address = address;
  m_config = ADS1115_COMP_QUEUE_AFTER_ONE |
            ADS1115_COMP_LATCH_NO |
            ADS1115_COMP_POLARITY_ACTIVE_LOW |
            ADS1115_COMP_MODE_WINDOW |
            ADS1115_DATA_RATE_128_SPS |
            ADS1115_MODE_SINGLE_SHOT |
            ADS1115_MUX_GND_AIN0;
  set_pga(ADS1115_PGA_ONE);
}

bool ADS1115::write_register(uint8_t reg, uint16_t val)
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (m_address << 1) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, val>>8, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, val & 0xFF, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  return ret == ESP_OK;

/*  Wire.beginTransmission(m_address);
  Wire.write(reg);
  Wire.write(val>>8);
  Wire.write(val & 0xFF);
  return Wire.endTransmission();
  */
}

uint16_t ADS1115::read_register(uint8_t reg)
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (m_address << 1) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (m_address << 1) | READ_BIT, ACK_CHECK_EN);
  uint8_t d0, d1;
  i2c_master_read_byte(cmd, &d0, I2C_MASTER_ACK);
  i2c_master_read_byte(cmd, &d1, I2C_MASTER_NACK);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  if (ret != ESP_OK)
  {
    ESP_LOGI("I2C", "Failed to read I2C: %d", ret);
    return 0;
  }

  return (d0 << 8) | d1;

/*  Wire.beginTransmission(m_address);
  Wire.write(reg);
  Wire.endTransmission();

  uint8_t result = Wire.requestFrom((int)m_address, 2, 1);
  if (result != 2) 
  {
    Wire.flush();
    return 0;
  }

  uint16_t val;

  val = Wire.read() << 8;
  val |= Wire.read();
  return val;
  */
}

void ADS1115::begin()
{

}

bool ADS1115::trigger_sample()
{
  return write_register(ADS1115_REGISTER_CONFIG, m_config | SAMPLE_BIT);
}

bool ADS1115::reset()
{
  //Wire.beginTransmission(0);
  //Wire.write(0x6);
  //return Wire.endTransmission();
  return false;
}

bool ADS1115::is_sample_in_progress()
{
  uint16_t val = read_register(ADS1115_REGISTER_CONFIG);
  return (val & SAMPLE_BIT) == 0;
}

int16_t ADS1115::read_sample()
{
  return read_register(ADS1115_REGISTER_CONVERSION);
}

float ADS1115::sample_to_float(int16_t val)
{
  return (val / FACTOR) * ranges[m_voltage_range];
}

float ADS1115::read_sample_float()
{
  int16_t sample = read_sample();
  //Serial.println(sample);
  //Serial.print(" ");
  return sample_to_float(sample);
}
