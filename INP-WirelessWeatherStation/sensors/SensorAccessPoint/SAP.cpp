#include "SAP.h"

SAP::SAP(){
  
}

void SAP::begin(uint16_t startupThreshold, float referencePressure){
  bme280.setReferencePressure(referencePressure);
  bme280.setMode(BME_MODE::FORCED);

  Wire.begin();
  uint32_t start = millis();

  while(true){
  	if(bme280.beginI2C()){
      status = STATUS::OK;
      break;
    }
  	if(millis() - start > startupThreshold){
      status = STATUS::BAD_WIRING_I2C_BME280;
      break;
    }
  }

  dht11.begin();
}

float SAP::getIlluminance(uint16_t millivolts){
  float illuminance = LDR_4V_215_2R(millivolts);
  if(illuminance == 100000){
    status = STATUS::BAD_ILLUMINANCE;
  }
  return LDR_4V_215_2R(millivolts);
}

float SAP::getPressure(){
	return bme280.readFloatPressure() / 100; // divide by 100 to get hecto Pascals
}

float SAP::getTemperature(){
	return bme280.readTempC(); // Broken Calibration: * 0.9629 - 4.1434
}

float SAP::getHumidity(){
  return dht11.readHumidity() * (100.0/93.0); // Calibration correction
}

// !!! PROBLEMATIC !!!
float SAP::getCarbonMonoxideConcentration(){
  return 0; // broken for the time being
}

// !!! PROBLEMATIC !!!
float SAP::getFCMvout(){ 
  return (float)fcm.measure(); 
}

// !!! PROBLEMATIC !!!
float SAP::getFCMrs(float rawVout, float voltage, uint16_t resolution, float RL /* RL >= 10kOhm */){ 
  float vout = rawVout * voltage / resolution;
  return voltage * RL / vout - RL;
}