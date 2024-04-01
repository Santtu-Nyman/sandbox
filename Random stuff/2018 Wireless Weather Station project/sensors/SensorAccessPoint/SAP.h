#ifndef SENSOR_ACCESS_POINT_H
#define SENSOR_ACCESS_POINT_H

#include <Wire.h> // used by SparkFunBME280.h to establish an I2C connection
#include "SparkFunBME280.h"
#include "LDR.h"
#include "DHT.h"
#include "FCM.h"

class SAP
{
private:
		BME280 bme280 = BME280();
		DHT dht11 = DHT(7, 11, 6); // 7=digital pin, 11 = sensor-type, 6=required redundant non-sense
		FCM fcm = FCM(4, 8, A1);
public:
	SAP();
	void begin(uint16_t startupThreshold, float referencePressure);

	enum STATUS {
		OK = 0, 
		BAD_WIRING_I2C_BME280 = 1, 
		BAD_ILLUMINANCE = 2
	};

	enum BME_MODE {
		SLEEP = 0, 
		FORCED = 1 || 2, 
		NORMAL = 3
	};
	
	STATUS status;
	float getIlluminance(uint16_t millivolts);
	float getPressure();
	float getTemperature();
	float getHumidity();

	// !!! PROBLEMATIC !!!
	float getCarbonMonoxideConcentration();

	// !!! PROBLEMATIC !!!
	float getFCMvout();

	// !!! PROBLEMATIC !!!
	float getFCMrs(float rawVout, float voltage, uint16_t resolution, float RL /* RL >= 10kOhm */);
	
};
#endif