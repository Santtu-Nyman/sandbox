/*
	MCP3201 Library version 1.0.0 2019-03-02 by Santtu Nyman.
	git https://github.com/AP-Elektronica-ICT/ip2019-coolwater
*/

#ifdef __cplusplus
extern "C" {
#endif
	
#include "cwd_mcp3201.h"

#if defined(linux) || defined(__linux) || defined(__linux__)

#define _GNU_SOURCE
#include <time.h>
#include "bcm2835.h"

static void cwd_mcp3201_read_wait_half_clock()
{
	const unsigned long long half_clock = 25000;
	struct timespec begin;
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &begin);
	now.tv_sec = begin.tv_sec;
	now.tv_nsec = begin.tv_nsec;
	while ((((unsigned long long)(now.tv_sec - begin.tv_sec) * 1000000000) + (unsigned long long)now.tv_nsec) - (unsigned long long)begin.tv_nsec < half_clock)
		clock_gettime(CLOCK_MONOTONIC, &now);
}

int cwd_mcp3201_read(uint8_t spi_cs, uint8_t spi_clk, uint8_t spi_dout)
{
	int value = 0;
	bcm2835_gpio_fsel(spi_cs, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(spi_clk, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(spi_dout, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_set_pud(spi_dout, BCM2835_GPIO_PUD_OFF);
	bcm2835_gpio_write(spi_cs, HIGH);
	bcm2835_gpio_write(spi_clk, LOW);
	cwd_mcp3201_read_wait_half_clock();
	cwd_mcp3201_read_wait_half_clock();
	bcm2835_gpio_write(spi_cs, LOW);
	for (int c = 3; c--;)
	{
		cwd_mcp3201_read_wait_half_clock();
		bcm2835_gpio_write(spi_clk, HIGH);
		cwd_mcp3201_read_wait_half_clock();
		bcm2835_gpio_write(spi_clk, LOW);
	}
	for (int c = 12; c--;)
	{
		cwd_mcp3201_read_wait_half_clock();
		value = (value << 1) | (int)bcm2835_gpio_lev(spi_dout);
		bcm2835_gpio_write(spi_clk, HIGH);
		cwd_mcp3201_read_wait_half_clock();
		bcm2835_gpio_write(spi_clk, LOW);
	}
	bcm2835_gpio_write(spi_cs, HIGH);
	return value;
}

#endif

#if defined(ARDUINO)

#include <arduino.h>

int cwd_mcp3201_read(uint8_t spi_cs, uint8_t spi_clk, uint8_t spi_dout)
{
	const unsigned int half_clock = 25;
	int value = 0;
	noInterrupts();
	pinMode(spi_cs, OUTPUT);
	pinMode(spi_clk, OUTPUT);
	pinMode(spi_dout, INPUT);
	digitalWrite(spi_cs, HIGH);
	digitalWrite(spi_clk, LOW);
	delayMicroseconds(2 * half_clock);
	digitalWrite(spi_cs, LOW);
	for (int c = 3; c--;)
	{
		delayMicroseconds(half_clock);
		digitalWrite(spi_clk, HIGH);
		delayMicroseconds(half_clock);
		digitalWrite(spi_clk, LOW);
	}
	for (int c = 12; c--;)
	{
		delayMicroseconds(half_clock);
		value = (value << 1) | (int)digitalRead(spi_dout);
		digitalWrite(spi_clk, HIGH);
		delayMicroseconds(half_clock);
		digitalWrite(spi_clk, LOW);
	}
	digitalWrite(spi_cs, HIGH);
	interrupts();
	return value;
}

#endif

#ifdef __cplusplus
}
#endif