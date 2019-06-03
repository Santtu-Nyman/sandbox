#include "LDR.h"
#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>
#include <avr/pgmspace.h>
// LDR must have 215.2ohm resistor in series with a voltage supply of 4V
const uint16_t voltBase[16] PROGMEM = {0, 1, 4, 29, 67, 100, 161, 235, 321, 548,  773, 1061, 1500,  1837, 2115, 2491};
const uint16_t luxBase[16] PROGMEM = {0, 1, 2,  5, 10,  20,  50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};
const float coefficents[16] PROGMEM = {
    0.0f,
    1.0f,
    0.333333f,
    0.120000f,
    1.0f / 7.6f,
    1.0f / 3.3f,
    1.0f / 2.033333f,
    1.0f / 1.48f,
    1.0f / 0.86f,
    1.0f / 0.756666f,
    1.0f / 0.45f,
    1.0f / 0.288f,
    1.0f / 0.146333f,
    1.0f / 0.0674f,
    1.0f / 0.0278f,
    1.0f / 0.012533f };

    
float LDR_4V_215_2R(uint16_t millivolts){
	if (millivolts == pgm_read_word_near((uint16_t)voltBase))
		return 0;

	for (uint8_t index = 1;; ++index)
	{
		float coeff = pgm_read_float_near((uint16_t)coefficents + (uint16_t)index * sizeof(float));
		uint16_t vb_curr = pgm_read_word_near((uint16_t)voltBase + (uint16_t)index * sizeof(uint16_t));
		uint16_t vb_prev = pgm_read_word_near((uint16_t)voltBase + (uint16_t)(index-1) * sizeof(uint16_t));
		float diff = millivolts - vb_prev;
		uint16_t lux = pgm_read_word_near((uint16_t)luxBase + (uint16_t)(index-1) * sizeof(uint16_t));

		if (millivolts <= vb_curr)
			return  coeff * diff + lux;
	}
	
	return 100000.0f;

}

	

/*
#include "LDR.h"
// LDR must have 215.2ohm resistor in series with a voltage supply of 4V
float LDR_4V_215_2R(uint16_t millivolts){
  const float coefficents[] = {
  	0,
  	1,
  	0.333333,
		0.120000,
		1.0 / 7.6,
		1.0 / 3.3,
		1.0 / 2.033333,
		1.0 / 1.48,
		1.0 / 0.86,
		1.0 / 0.756666,
		1.0 / 0.45,
		1.0 / 0.288,
		1.0 / 0.146333,
		1.0 / 0.0674,
		1.0 / 0.0278,
		1.0 / 0.012533
	};

	const uint16_t voltBase[] = {0, 1, 4, 29, 67, 100, 161, 235, 321, 548,  773, 1061, 1500,  1837, 2115, 2491};
	const uint16_t luxBase[]  = {0, 1, 2,  5, 10,  20,  50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};

			 if(millivolts == voltBase[0])  return coefficents[0]  * (millivolts-voltBase[0])  + luxBase[0];
	else if(millivolts <= voltBase[1])  return coefficents[1]  * (millivolts-voltBase[0])  + luxBase[0];
	else if(millivolts <= voltBase[2])  return coefficents[2]  * (millivolts-voltBase[1])  + luxBase[1];
	else if(millivolts <= voltBase[3])  return coefficents[3]  * (millivolts-voltBase[2])  + luxBase[2];
	else if(millivolts <= voltBase[4])  return coefficents[4]  * (millivolts-voltBase[3])  + luxBase[3];
	else if(millivolts <= voltBase[5])  return coefficents[5]  * (millivolts-voltBase[4])  + luxBase[4];
	else if(millivolts <= voltBase[6])  return coefficents[6]  * (millivolts-voltBase[5])  + luxBase[5];
	else if(millivolts <= voltBase[7])  return coefficents[7]  * (millivolts-voltBase[6])  + luxBase[6];
	else if(millivolts <= voltBase[8])  return coefficents[8]  * (millivolts-voltBase[7])  + luxBase[7];
	else if(millivolts <= voltBase[9])  return coefficents[9]  * (millivolts-voltBase[8])  + luxBase[8];
	else if(millivolts <= voltBase[10]) return coefficents[10] * (millivolts-voltBase[9])  + luxBase[9];
	else if(millivolts <= voltBase[11]) return coefficents[11] * (millivolts-voltBase[10]) + luxBase[10];
	else if(millivolts <= voltBase[12]) return coefficents[12] * (millivolts-voltBase[11]) + luxBase[11];
	else if(millivolts <= voltBase[13]) return coefficents[13] * (millivolts-voltBase[12]) + luxBase[12];
	else if(millivolts <= voltBase[14]) return coefficents[14] * (millivolts-voltBase[13]) + luxBase[13];
	else if(millivolts <= voltBase[15]) return coefficents[15] * (millivolts-voltBase[14]) + luxBase[14];
	else return 100000;
}*/