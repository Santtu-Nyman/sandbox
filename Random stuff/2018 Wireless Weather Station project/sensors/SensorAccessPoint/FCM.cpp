#include "FCM.h"

#define TIMING_DEBUG FALSE

FCM::FCM(uint8_t heaterPulsePin, 
				 uint8_t circuitPulsePin, 
				 uint8_t readingPin) : heaterPulsePin(heaterPulsePin),
															 circuitPulsePin(circuitPulsePin),
															 readingPin(readingPin){
																												pinMode(heaterPulsePin, OUTPUT);
																												pinMode(circuitPulsePin, OUTPUT);
																												pinMode(readingPin, INPUT);
																										 }

uint16_t FCM::measure(){
	#if TIMING_DEBUG
	uint32_t start = millis();
	uint32_t delay14;
	uint32_t delay995;
	uint32_t delay997;
	uint32_t delay1000;
	#endif

	digitalWrite(heaterPulsePin, LOW);  // apply 5V to heater element for 14ms
	digitalWrite(circuitPulsePin, LOW); // apply 0V to circuit behind load resistor on the GND side for 995ms (14ms + 981ms delays)
	delay(14); // elapsed 14ms

	#if TIMING_DEBUG
	delay14 = millis() - start;
	#endif

	digitalWrite(heaterPulsePin, HIGH); // apply 0V to heater element for 986ms
	delay(981); // elapsed 995ms

	#if TIMING_DEBUG
	delay995 = millis() - start;
	#endif

	digitalWrite(circuitPulsePin, HIGH); // apply 5V to circuit behind load resistor on the GND side for 5ms
	delay(2); // wait for 2ms in order to get the most precise measurement in the middle of the 5ms measurement window, elapsed 997ms

	#if TIMING_DEBUG
	delay997 = millis() - start;
	#endif

	uint16_t result = analogRead(readingPin); // take the measurement
	digitalWrite(circuitPulsePin, LOW); 			// reapply 0V to circuit behind load resistor on the GND side
	delay(3); 																// elapsed 1000ms

	#if TIMING_DEBUG
	delay1000 = millis() - start;
	Serial.print("14ms -> ");
	Serial.print(delay14);
	Serial.print("ms, ");
	Serial.print("995ms -> ");
	Serial.print(delay995);
	Serial.print("ms, ");
	Serial.print("997ms -> ");
	Serial.print(delay997);
	Serial.print("ms, ");
	Serial.print("1000ms -> ");
	Serial.print(delay1000);
	Serial.print("ms, ");
	#endif
	
	return result;
}