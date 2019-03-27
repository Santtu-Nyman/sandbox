#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>
#include "inp_power.h"
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

void inp_configure_power()
{
	pinMode(MAIN_REGULATOR_DISABLE_PIN, OUTPUT);
	digitalWrite(MAIN_REGULATOR_DISABLE_PIN, LOW);
	pinMode(MCU_POWER_DISABLE_PIN, OUTPUT);
	digitalWrite(MCU_POWER_DISABLE_PIN, LOW);
	pinMode(ENABLE_4V_3_3V_PIN, OUTPUT);
	digitalWrite(ENABLE_4V_3_3V_PIN, LOW);
	pinMode(GPRS_RESET_PIN, OUTPUT);
	digitalWrite(GPRS_RESET_PIN, LOW);
	pinMode(SENSOR_ENABLE_PIN, OUTPUT);
	digitalWrite(SENSOR_ENABLE_PIN, LOW);
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, LOW);

  digitalWrite(ENABLE_4V_3_3V_PIN, HIGH);
  digitalWrite(SENSOR_ENABLE_PIN, HIGH);
}

ISR (WDT_vect) 
{
   wdt_disable();
}

void inp_sleep(unsigned long time)
{
  digitalWrite(ENABLE_4V_3_3V_PIN, LOW);
  digitalWrite(SENSOR_ENABLE_PIN, LOW);
  digitalWrite(LED_BUILTIN, LOW);
  
  power_adc_disable(); // ADC converter
  power_spi_disable(); // SPI
  power_usart0_disable();// Serial (USART)
  power_timer0_disable();// Timer 0
  power_timer1_disable();// Timer 1
  power_timer2_disable();// Timer 2
  power_twi_disable(); // TWI (I2C)

  while (time)
  {
    if (time > 8015)
    {
      digitalWrite(MCU_POWER_DISABLE_PIN, HIGH);
      __asm__ __volatile__ ("nop");
      digitalWrite(MAIN_REGULATOR_DISABLE_PIN, HIGH);
      MCUSR = 0;     
      WDTCSR = (1 << WDCE) | (1 << WDE);
      WDTCSR = (1 << WDIE) | (1<<WDP3 )|(0<<WDP2 )|(0<<WDP1)|(1<<WDP0);// 8 S
      wdt_reset();
      set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
      noInterrupts();
      sleep_enable();
      MCUCR = (1 << BODS) | (1 << BODSE);
      MCUCR = (1 << BODS); 
      interrupts();
      sleep_cpu();
      sleep_disable();
      time -= 8016;
    }
    if (time > 4015)
    {
      digitalWrite(MCU_POWER_DISABLE_PIN, HIGH);
      __asm__ __volatile__ ("nop");
      digitalWrite(MAIN_REGULATOR_DISABLE_PIN, HIGH);
      MCUSR = 0;     
      WDTCSR = (1 << WDCE) | (1 << WDE);
      WDTCSR = (1 << WDIE) | (1 << WDP3);// 8 S
      wdt_reset();
      set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
      noInterrupts();
      sleep_enable();
      MCUCR = (1 << BODS) | (1 << BODSE);
      MCUCR = (1 << BODS); 
      interrupts();
      sleep_cpu();
      sleep_disable();
      time -= 4016;
    }
    if (time > 2015)
    {
      digitalWrite(MCU_POWER_DISABLE_PIN, HIGH);
      __asm__ __volatile__ ("nop");
      digitalWrite(MAIN_REGULATOR_DISABLE_PIN, HIGH);
      MCUSR = 0;     
      WDTCSR = (1 << WDCE) | (1 << WDE);
      WDTCSR = (1 << WDIE) | (1 << WDP2) | (1 << WDP1) | (1 << WDP0);// 2 S
      wdt_reset();
      set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
      noInterrupts();
      sleep_enable();
      MCUCR = (1 << BODS) | (1 << BODSE);
      MCUCR = (1 << BODS); 
      interrupts();
      sleep_cpu();
      sleep_disable();
      time -= 2016;
    }
    else
    {
      digitalWrite(MCU_POWER_DISABLE_PIN, HIGH);
      __asm__ __volatile__ ("nop");
      digitalWrite(MAIN_REGULATOR_DISABLE_PIN, HIGH);
      MCUSR = 0;     
      WDTCSR = (1 << WDCE) | (1 << WDE);
      WDTCSR = (1 << WDIE) | (1 << WDP2) | (1 << WDP1);// 1 S
      wdt_reset();
      set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
      noInterrupts();
      sleep_enable();
      MCUCR = (1 << BODS) | (1 << BODSE);
      MCUCR = (1 << BODS); 
      interrupts();
      sleep_cpu();
      sleep_disable();
      time = 0;
    }
    digitalWrite(MAIN_REGULATOR_DISABLE_PIN, LOW);
    __asm__ __volatile__ ("nop");
    digitalWrite(MCU_POWER_DISABLE_PIN, LOW);
    MCUSR = 0;     
    WDTCSR = (1 << WDCE) | (1 << WDE);
    WDTCSR = (1 << WDIE) | (0<<WDP3 )|(0<<WDP2 )|(0<<WDP1)|(0<<WDP0);// 16 mS
    wdt_reset();
    set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
    noInterrupts();
    sleep_enable();
    MCUCR = (1 << BODS) | (1 << BODSE);
    MCUCR = (1 << BODS); 
    interrupts();
    sleep_cpu();
    sleep_disable();
  }
  power_adc_enable(); // ADC converter
  power_spi_enable(); // SPI
  power_usart0_enable(); // Serial (USART)
  power_timer0_enable(); // Timer 0
  power_timer1_enable(); // Timer 1
  power_timer2_enable(); // Timer 2
  power_twi_enable(); // TWI (I2C)

  digitalWrite(ENABLE_4V_3_3V_PIN, HIGH);
  digitalWrite(SENSOR_ENABLE_PIN, HIGH);
}

