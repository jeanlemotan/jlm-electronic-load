#include "PWM.h"
#include <Arduino.h>

// Configure the PWM clock
// The argument is one of the 5 previously defined modes
void pwm1Configure(uint8_t mode)
{
  // TCCR1A configuration
  TCCR1A = bit(WGM11);

  // TCCR1B configuration
  // Clock mode and Fast PWM 8 bit
  TCCR1B = mode | bit(WGM13) | bit(WGM12);

  // TCCR1C configuration
  TCCR1C = 0;
  ICR1 = 65535; //top
}

// Set PWM to D9
// Argument is PWM between 0 and 255
void pwm1Set9(uint16_t value)
{
  OCR1A = value;   // Set PWM value
  DDRB |= 1<<5;    // Set Output Mode B5
  TCCR1A |= 0x80;  // Activate channel
}

// Set PWM to D10
// Argument is PWM between 0 and 255
void pwm1Set10(uint16_t value)
{
  OCR1B = value;   // Set PWM value
  DDRB |= 1<<6;    // Set Output Mode B6
  TCCR1A |= 0x20;  // Set PWM value
}

// Set PWM to D11
// Argument is PWM between 0 and 255
void pwm1Set11(uint16_t value)
{
  OCR1C = value;   // Set PWM value
  DDRB |= 1<<7;    // Set Output Mode B7
  TCCR1A |= 0x08;  // Set PWM value
}

// Configure the PWM clock
// The argument is one of the 7 previously defined modes
void pwm4Configure(uint8_t mode)
{
  TCCR4A = 0;
  TCCR4B = mode;
  TCCR4C = 0;
  TCCR4D = 0;
  TCCR4D = 0;
  
  // PLL Configuration
  // Use 96MHz / 2 = 48MHz
  PLLFRQ=(PLLFRQ&0xCF)|0x30;
  // PLLFRQ=(PLLFRQ&0xCF)|0x10; // Will double all frequencies
  
  // Terminal count for Timer 4 PWM
  OCR4C=255;
}

// Set PWM to D6 (Timer4 D)
// Argument is PWM between 0 and 255
void pwm4Set6(uint8_t value)
{
  OCR4D = value;   // Set PWM value
  DDRD |= 1<<7;    // Set Output Mode D7
  TCCR4C |= 0x09;  // Activate channel D
}

