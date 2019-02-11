/*
	Cool Water Dispenser user interface version (version number).(added feature number).(pacth number) written by (your name).
	git repository https://github.com/AP-Elektronica-ICT/ip2019-coolwater
	
	Description
		TODO: Add file description
*/

#ifndef CWD_UI_H
#define CWD_UI_H

#include <stddef.h>
#include <stdint.h>
#include <errno.h>

#define CWD_UI_ULTRASONIC_SENSOR_COUNT 3
#define CWD_UI_INDICATOR_DEVICE_OPERATIONAL 0
#define CWD_UI_INDICATOR_ULTRA_0 1
#define CWD_UI_INDICATOR_ULTRA_1 2
#define CWD_UI_INDICATOR_ULTRA_2 3
#define CWD_UI_INDICATOR_LIGHT_COUNT 4
#define CWD_UI_COLOR_OFF 0
#define CWD_UI_COLOR_RED 1
#define CWD_UI_COLOR_GREEN 2
#define CWD_UI_COLOR_BLUE 3
// add more colors if needed

int cwd_ui_initialize();
/*
	Description
		This function initialize the user interface of the device.
		All indicator light are turned off when the device user interface initializes.
		This function will fail with error code EALREADY if device is already initialized.
	Parameters
		Function has no parameters.
	Return
		Function returns zero on success and errno value on failure.
*/

void cwd_ui_shutdown();
/*
	Description
		This function frees resources used by the device user interface.
	Parameters
		Function has no parameters.
	Return
		Function has no return value.
*/

int cwd_ui_read_ultrasonic_sensor(size_t sensor_number, float* measured_distance);
/*
	Description
		This function measures the distance to nearest object in front of the ultarsonic sensor
		specified by sensor_number parameter in meters. and writes it to variable pointed
		by measured_distance if procedure was succsesful.
	Parameters
		sensor_number
			Number of the sensor to be read.
		measured_distance
			pointer to variable that receives measured distance in meters.
	Return
		Function returns zero on success and errno value on failure.
*/

int cwd_ui_set_indicator_light(size_t light_number, int color);
/*
	Description
		This function sets the color of indicator light to color specified by color parameter.
	Parameters
		light_number
			Number of the light to be set to specific color.
		color
			color for the light.
	Return
		Function returns zero on success and errno value on failure.
*/

int cwd_ui_get_drink_order(size_t* order_number);
/*
	Description
		This function figure out somehow what drink was ordered (probably using cwd_ui_read_ultrasonic_sensor and timing)
		TODO: Add description that is not trash.
	Parameters
		order_number
			pointer to variable that receives order number.
	Return
		Function returns zero on success and errno value on failure.
		EAGAIN is returned when the procedure was succsessful but no drink was ordered.
*/

#endif