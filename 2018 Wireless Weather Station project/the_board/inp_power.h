#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

#define MAIN_REGULATOR_DISABLE_PIN 2
#define MCU_POWER_DISABLE_PIN 3
#define ENABLE_4V_3_3V_PIN 4
#define GPRS_RESET_PIN 5
#define SENSOR_ENABLE_PIN 6

void inp_configure_power();

void inp_sleep(unsigned long time);