// AiEsp32RotaryEncoder.h
// based on https://github.com/marcmerlin/IoTuz code - extracted and modified Encoder code

#pragma once

#include "Arduino.h"
#include "RotaryEncoder.h"

// Rotary Encocer
#define AIESP32ROTARYENCODER_DEFAULT_A_PIN 25
#define AIESP32ROTARYENCODER_DEFAULT_B_PIN 26
#define AIESP32ROTARYENCODER_DEFAULT_BUT_PIN 15
#define AIESP32ROTARYENCODER_DEFAULT_VCC_PIN -1

class AiEsp32RotaryEncoder : public RotaryEncoder
{
public: 
	AiEsp32RotaryEncoder(
		uint8_t encoderAPin = AIESP32ROTARYENCODER_DEFAULT_A_PIN,
		uint8_t encoderBPin = AIESP32ROTARYENCODER_DEFAULT_B_PIN,
		uint8_t encoderButtonPin = AIESP32ROTARYENCODER_DEFAULT_BUT_PIN,
		uint8_t encoderVccPin = AIESP32ROTARYENCODER_DEFAULT_VCC_PIN
	);
	void setBoundaries(int16_t minValue = -100, int16_t maxValue = 100, bool circleValues = false) override;
	void IRAM_ATTR readEncoder_ISR();
	
	void setup(void (*ISR_callback)(void));
	void begin();
	void reset(int16_t newValue = 0) override;
	void enable() override;
	void disable() override;
	int16_t encoderDelta() override;
	int16_t encoderDeltaAcc() override;
	ButtonState buttonState() override;

private:
	int16_t readEncoder();

	portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
	volatile int16_t encoder0Pos = 0;
	bool _circleValues = false;
	bool isEnabled = true;

	uint8_t encoderAPin      = AIESP32ROTARYENCODER_DEFAULT_A_PIN;
	uint8_t encoderBPin      = AIESP32ROTARYENCODER_DEFAULT_B_PIN;
	uint8_t encoderButtonPin = AIESP32ROTARYENCODER_DEFAULT_BUT_PIN;
	uint8_t encoderVccPin    = AIESP32ROTARYENCODER_DEFAULT_VCC_PIN;

	int16_t _minEncoderValue = -(1 << 15);
	int16_t _maxEncoderValue = 1 << 15;

	uint8_t old_AB;
	int16_t lastReadEncoder0Pos;
	bool previous_butt_state;
	uint32_t lastAccTP = millis();

	int8_t enc_states[16] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
	void(*ISR_callback)();
};

