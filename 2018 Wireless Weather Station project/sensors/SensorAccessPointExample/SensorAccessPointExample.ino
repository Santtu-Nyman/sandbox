#include "SAP.h"

SAP sap;

const uint8_t LDR_VOLTAGE_SUPPLY_PIN = 3;  // !!! NOT TO BE USED IN THE FINAL INTEGRATION !!!
const uint8_t LDR_VOLTAGE_OUTPUT_PIN = A0; // !!! NOT TO BE USED IN THE FINAL INTEGRATION !!!

void setup(){
  Serial.begin(9600); // !!! NOT TO BE USED IN THE FINAL INTEGRATION !!!
  while(!Serial);     // !!! NOT TO BE USED IN THE FINAL INTEGRATION !!!

  sap.begin(1000, 102350);
  pinMode(LDR_VOLTAGE_SUPPLY_PIN, OUTPUT); // !!! NOT TO BE USED IN THE FINAL INTEGRATION !!!
  pinMode(LDR_VOLTAGE_OUTPUT_PIN, INPUT);  // !!! NOT TO BE USED IN THE FINAL INTEGRATION !!!
}

const uint16_t PWM_4_006V = 967; 								// !!! NOT TO BE USED IN THE FINAL INTEGRATION !!!
const uint16_t RESOLUTION = 1024; 							// !!! NOT TO BE USED IN THE FINAL INTEGRATION !!!
const float PWM_4_006V_AS_MILLIVOLTS = 4006.0f; // !!! NOT TO BE USED IN THE FINAL INTEGRATION !!!

float LDR_mV = 0.0f;
float pressure = 0.0f;
float temperature = 0.0f;
float illuminance = 0.0f;
float humidity = 0.0f;
float coppm = 0.0f; // !!! PROBLEMATIC !!!
float vout = 0.0f; 	// !!! PROBLEMATIC !!!
float rs = 0.0f; 		// !!! PROBLEMATIC !!!

void loop(){
  // dht11 sensor requires 2second delays
  // here for the sake of simplicity, let's delay all sensors by 2 seconds
  delay(2000); 

  // using PWM to provide 4V supply for LDR
  analogWrite(LDR_VOLTAGE_SUPPLY_PIN, PWM_4_006V); // !!! NOT TO BE USED IN THE FINAL INTEGRATION !!!

  do {
  	// calculate the voltage between LDR and LDR series resistor (series resistor leads to GND)
  	LDR_mV = (float)analogRead(LDR_VOLTAGE_OUTPUT_PIN) / RESOLUTION * PWM_4_006V_AS_MILLIVOLTS;
  } while(!LDR_mV);
  
  // calculate and return the illuminance as lux
  illuminance = sap.getIlluminance(LDR_mV);

  // calculate and return pressure as hectoPascals
  pressure = sap.getPressure();

  // calculate and return temperature as Celsius
  temperature = sap.getTemperature();

  // calculate and return humiduty as percentage
  humidity = sap.getHumidity();

  // calculate and return carbon monoxide concentration in the air as parts per million (ppm)
  coppm = sap.getCarbonMonoxideConcentration(); 	 // !!! PROBLEMATIC !!!

  // debug raw carbon monoxide sensor vout value
  vout = sap.getFCMvout(); 												 // !!! PROBLEMATIC !!!

  rs = sap.getFCMrs(vout, 5.000f, 1024, 39000.0f); // !!! PROBLEMATIC !!!

  // print sensor readings in meaningful units
  Serial.print(LDR_mV, 2);			// !!! NOT TO BE USED IN THE FINAL INTEGRATION !!!
  Serial.print(" LDR mV | ");		// !!! NOT TO BE USED IN THE FINAL INTEGRATION !!!

  Serial.print(illuminance, 2);	// !!! NOT TO BE USED IN THE FINAL INTEGRATION !!!
  Serial.print(" lx | ");				// !!! NOT TO BE USED IN THE FINAL INTEGRATION !!!

  Serial.print(pressure, 2);		// !!! NOT TO BE USED IN THE FINAL INTEGRATION !!!
  Serial.print(" hPa | ");			// !!! NOT TO BE USED IN THE FINAL INTEGRATION !!!

  Serial.print(temperature, 2);	// !!! NOT TO BE USED IN THE FINAL INTEGRATION !!!
  Serial.print(" °C | ");				// !!! NOT TO BE USED IN THE FINAL INTEGRATION !!!

  Serial.print(humidity, 2);		// !!! NOT TO BE USED IN THE FINAL INTEGRATION !!!
  Serial.print(" RH % | ");			// !!! NOT TO BE USED IN THE FINAL INTEGRATION !!!

  /*Serial.print(coppm, 6);					// !!! PROBLEMATIC !!!
  Serial.print(" CO ppm | "); */		// !!! PROBLEMATIC !!!

  Serial.print(vout, 2);						// !!! PROBLEMATIC !!!
  Serial.print(" CO vout raw | "); 	// !!! PROBLEMATIC !!!

  Serial.print(rs);									// !!! PROBLEMATIC !!!
  Serial.println(" CO rs ohm");			// !!! PROBLEMATIC !!!

}
