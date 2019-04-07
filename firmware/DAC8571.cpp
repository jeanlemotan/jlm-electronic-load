#include <Arduino.h>
#include <Wire.h>
#include "DAC8571.h"

DAC8571::DAC8571(uint8_t address)
{
  m_address = address;
}

uint8_t DAC8571::write_register(uint8_t reg, uint16_t val)
{
  Wire.beginTransmission(m_address);
  Wire.write(reg);
  Wire.write(val>>8);
  Wire.write(val & 0xFF);
  return Wire.endTransmission();
}

uint16_t DAC8571::read_register(uint8_t reg)
{
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
}

void DAC8571::begin()
{
  Wire.begin();

  //switch to fast mode
  Wire.beginTransmission(m_address);

  uint8_t cmd = bit(4) | bit(0);
  Wire.write(cmd);

  uint8_t msb = bit(5);
  Wire.write(msb);
  uint8_t lsb = 0;
  Wire.write(lsb);
  Wire.endTransmission();
}

uint8_t DAC8571::write(uint16_t data)
{
  Wire.beginTransmission(m_address);

  uint8_t cmd = bit(4);
  Wire.write(cmd);

  Wire.write(data >> 8);
  Wire.write(data & 0xFF);
  return Wire.endTransmission();
}
