#include "ADS1115.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>

Adafruit_SSD1351 s_display = Adafruit_SSD1351(5, 1, 0);

// Frequency modes for TIMER1
#define PWM1_62k   1   //62500 Hz
#define PWM1_8k    2   // 7812 Hz
#define PWM1_1k    3   //  976 Hz
#define PWM1_244   4   //  244 Hz
#define PWM1_61    5   //   61 Hz

// Direct PWM change variables
#define PWM9   OCR1A
#define PWM10  OCR1B
#define PWM11  OCR1C

// Configure the PWM clock
// The argument is one of the 5 previously defined modes
void pwm1Configure(int mode)
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
void pwm1Set10(int value)
{
  OCR1B = value;   // Set PWM value
  DDRB |= 1<<6;    // Set Output Mode B6
  TCCR1A |= 0x20;  // Set PWM value
}

// Set PWM to D11
// Argument is PWM between 0 and 255
void pwm1Set11(int value)
{
  OCR1C = value;   // Set PWM value
  DDRB |= 1<<7;    // Set Output Mode B7
  TCCR1A |= 0x08;  // Set PWM value
}

// Frequency modes for TIMER4
#define PWM4_187k 1   // 187500 Hz
#define PWM4_94k  2   //  93750 Hz
#define PWM4_47k  3   //  46875 Hz
#define PWM4_23k  4   //  23437 Hz
#define PWM4_12k  5   //  11719 Hz
#define PWM4_6k   6   //   5859 Hz
#define PWM4_3k   7   //   2930 Hz

// Direct PWM change variables
#define PWM6        OCR4D
#define PWM13       OCR4A

// Terminal count
#define PWM6_13_MAX OCR4C

// Configure the PWM clock
// The argument is one of the 7 previously defined modes
void pwm4Configure(int mode)
{
  // TCCR4A configuration
  TCCR4A=0;
  
  // TCCR4B configuration
  TCCR4B=mode;
  
  // TCCR4C configuration
  TCCR4C=0;
  
  // TCCR4D configuration
  TCCR4D=0;
  
  // TCCR4D configuration
  TCCR4D=0;
  
  // PLL Configuration
  // Use 96MHz / 2 = 48MHz
  PLLFRQ=(PLLFRQ&0xCF)|0x30;
  // PLLFRQ=(PLLFRQ&0xCF)|0x10; // Will double all frequencies
  
  // Terminal count for Timer 4 PWM
  OCR4C=255;
}

// Set PWM to D6 (Timer4 D)
// Argument is PWM between 0 and 255
void pwm4Set6(int value)
{
  OCR4D=value;   // Set PWM value
  DDRD|=1<<7;    // Set Output Mode D7
  TCCR4C|=0x09;  // Activate channel D
}


ADS1115 s_adc;

// the setup function runs once when you press reset or power the board
void setup() 
{
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(57600);
  pwm1Configure(PWM1_62k);

  pinMode(1, OUTPUT);
  
  s_adc.begin();
  s_adc.set_data_rate(ADS1115_DATA_RATE_250_SPS);
  s_adc.set_mode(ADS1115_MODE_CONTINUOUS);
  s_adc.set_mux(ADS1115_MUX_DIFF_AIN0_AIN1);
  s_adc.set_pga(ADS1115_PGA_EIGHT);
  if (s_adc.trigger_sample() != 0)
  {
      Serial.println("adc read trigger failed (ads1115 not connected?)");
  }

  pwm4Configure(PWM4_23k);
  pwm4Set6(128);

  s_display.begin();
  s_display.fillRect(0, 0, 128, 128, 0x0000);
}

// the loop function runs over and over again forever
void loop() {
  static uint16_t dac = 0;
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
  delay(3);                       // wait for a second

  static float adc = 0;
  {
    float val = s_adc.read_sample_float();
    if (val < 0)
    {
      val = 0;
    }
    adc = (adc * 99.f + val) / 100.f;
  }

  uint16_t step = 1;
  /*
  if (abs(adc - target) > 0.0002f)
  {
    step = 10;
  }
  else if (abs(adc - target) > 0.0001f)
  {
    step = 5;
  }
  else if (abs(adc - target) > 0.00005f)
  {
    step = 2;
  }
  else
  {
    step = 1;
  }*/

  pwm1Set9(dac);

  static int32_t lastTP = millis();
  if (lastTP + 100 < millis())
  {
    lastTP = millis();
    char buffer[128];
    sprintf(buffer, "DAC: %ld, ADC: %lduv", (int32_t)(dac), int32_t(adc * 1.093087316f * 1000000.f));
    Serial.println(buffer);
    s_display.setTextSize(1.5);
    s_display.setCursor(0,0);
    s_display.setTextColor(0xF800);
    s_display.print(buffer);
  }
}
