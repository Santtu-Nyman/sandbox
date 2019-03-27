#ifndef LDR_H
#define LDR_H
#include <stdint.h>
// ldr must have 215.2ohm resistor in series with a voltage supply of 4V
float LDR_4V_215_2R(uint16_t millivolts);
#endif
