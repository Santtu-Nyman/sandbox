#ifndef FIGARO_TGS2442_CARBON_MONOXIDE_H
#define FIGARO_TGS2442_CARBON_MONOXIDE_H

#include <stdint.h>
#include "Arduino.h"

class FCM
{
public:
	FCM(uint8_t heaterPulsePin, uint8_t circuitPulsePin, uint8_t readingPin);
	uint16_t measure();
private:
	uint8_t heaterPulsePin;
	uint8_t circuitPulsePin;
	uint8_t readingPin;
};

#endif