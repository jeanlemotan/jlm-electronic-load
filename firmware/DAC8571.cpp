#include <Arduino.h>
//#include <Wire.h>
#include <driver/i2c.h>
#include "DAC8571.h"

#define WRITE_BIT                          I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT                           I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN                       0x1              /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS                      0x0              /*!< I2C master will not check ack from slave */

DAC8571::DAC8571(uint8_t address)
{
  m_address = address;
}

uint8_t DAC8571::write_register(uint8_t reg, uint16_t val)
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

  /*
  Wire.beginTransmission(m_address);
  Wire.write(reg);
  Wire.write(val>>8);
  Wire.write(val & 0xFF);
  return Wire.endTransmission();
  */
}

uint16_t DAC8571::read_register(uint8_t reg)
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
  /*
  Wire.beginTransmission(m_address);
  Wire.write(reg);
  Wire.endTransmission();

  uint8_t result = Wire.requestFrom((int)m_address, 2, 1);
  if (result != 2) {
    Wire.flush();
    return 0;
  }

  uint16_t val;

  val = Wire.read() << 8;
  val |= Wire.read();
  return val;
  */
}

void DAC8571::begin()
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (m_address << 1) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, bit(4) | bit(0), ACK_CHECK_EN);
  i2c_master_write_byte(cmd, bit(5), ACK_CHECK_EN);
  i2c_master_write_byte(cmd, 0, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  ESP_ERROR_CHECK(ret);

  //switch to fast mode
  /*
  Wire.beginTransmission(m_address);

  uint8_t cmd = bit(4) | bit(0);
  Wire.write(cmd);

  uint8_t msb = bit(5);
  Wire.write(msb);
  uint8_t lsb = 0;
  Wire.write(lsb);
  Wire.endTransmission();
  */
}

uint8_t DAC8571::write(uint16_t data)
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (m_address << 1) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, bit(4), ACK_CHECK_EN);
  i2c_master_write_byte(cmd, data >> 8, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, data & 0xFF, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  return ret == ESP_OK;  
  /*
  Wire.beginTransmission(m_address);

  uint8_t cmd = bit(4);
  Wire.write(cmd);

  Wire.write(data >> 8);
  Wire.write(data & 0xFF);
  return Wire.endTransmission();
  */
}
