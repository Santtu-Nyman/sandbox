/*
	Cool Water Dispenser user interface version (version number).(added feature number).(pacth number) written by (your name).
	git repository https://github.com/AP-Elektronica-ICT/ip2019-coolwater
*/

#ifndef CWD_UI_H
#define CWD_UI_H

#include "cwd_ui.h"
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include "cwd_sbcm2835.h"

static int validate_color_value(int color);

static int validate_color_value(int color)
{
	if (color == CWD_UI_COLOR_OFF ||
		color == CWD_UI_COLOR_RED ||
		color == CWD_UI_COLOR_GREEN ||
		color == CWD_UI_COLOR_BLUE)
	{
		return 1;	
	}
	return 0;
}

int cwd_ui_initialize()
{
	cwd_sbcm2835_initialize();
	
	// test code
	bcm2835_gpio_fsel(2, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(2, HIGH);
	
	// inser implementation here
	return ENOSYS;// return error code on failure
}

void cwd_ui_shutdown()
{
	// more test code
	bcm2835_gpio_fsel(2, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(2, BCM2835_GPIO_PUD_OFF);
	
	cwd_sbcm2835_close();
	// inser implementation here
}

int cwd_ui_read_ultrasonic_sensor(size_t sensor_number, float* measured_distance)
{
	if (sensor_number >= CWD_UI_ULTRASONIC_SENSOR_COUNT)
	{
		return EINVAL;
	}
	// inser implementation here
	*measured_distance = 666.0f;// give fake measurement
	return ENOSYS;// return error code on failure
}

int cwd_ui_set_indicator_light(size_t light_number, int color)
{
	if (light_number >= CWD_UI_INDICATOR_LIGHT_COUNT || !validate_color_value(color))
	{
		return EINVAL;
	}
	// is device operational state light green?
	if (light_number == CWD_UI_INDICATOR_DEVICE_OPERATIONAL && !(color == CWD_UI_COLOR_OFF || color == CWD_UI_COLOR_GREEN))
	{
		return EINVAL;
	}
	// inser implementation here
	return ENOSYS;// return error code on failure
}

int cwd_ui_get_drink_order(size_t* order_number)
{
	// inser implementation here
	return ENOPKG;//i have no idea
}

#endif