#pragma once

#include <stdint.h>

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
void pwm1Configure(uint8_t mode);
// Set PWM to D9
// Argument is PWM between 0 and 255
void pwm1Set9(uint16_t value);
// Set PWM to D10
// Argument is PWM between 0 and 255
void pwm1Set10(uint16_t value);
// Set PWM to D11
// Argument is PWM between 0 and 255
void pwm1Set11(uint16_t value);


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
void pwm4Configure(uint8_t mode);

// Set PWM to D6 (Timer4 D)
// Argument is PWM between 0 and 255
void pwm4Set6(uint8_t value);

