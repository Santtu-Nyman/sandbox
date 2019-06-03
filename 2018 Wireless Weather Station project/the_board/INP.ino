#include <Arduino.h>
#include <string.h>
#include "inp_power.h"
#include "at_util.h"
#include "DHT.h"
#include "LDR.h"
#include "FCM.h"
#include <avr/pgmspace.h>
#define MS_TIME_MIMUTE 60000
#define MS_TIME_3_HOURS 10800000

#define LDR_VOLTAGE_OUTPUT_PIN A1      // !!! NOT TO BE USED IN THE FINAL INTEGRATION !!!
#define LDR_ADC_RESOLUTION 1024       // !!! NOT TO BE USED IN THE FINAL INTEGRATION !!!
#define PWM_4_006V_AS_MILLIVOLTS 4120.0f // !!! NOT TO BE USED IN THE FINAL INTEGRATION !!!

#define station_id 4
static uint32_t sleep_time;

#include <OneWire.h>
#include <DallasTemperature.h>

void setup()
{
  inp_configure_power();
  pinMode(LDR_VOLTAGE_OUTPUT_PIN, INPUT);
  Serial.begin(9600);
  sleep_time = MS_TIME_3_HOURS;
}

//#include <SoftwareSerial.h>

void loop()
{
  /*
  SoftwareSerial my_serial(7, 12); // RX, TX
  my_serial.begin(9600);
  */
  
  delay(1000);
  OneWire one_wire(A0);
  DallasTemperature dsb(&one_wire);
  dsb.begin();
  dsb.requestTemperatures();
  
  // calculate and return temperature as Celsius
  //float temperature = sap.getTemperature();
  float temperature = dsb.getTempCByIndex(0);
  /*
  my_serial.print("temperature ");
  my_serial.print(temperature);
  my_serial.print("\n");
  */

  // calculate and return humiduty as percentage
  //float humidity = sap.getHumidity();

  DHT dht11(11, 11, 6);
  dht11.begin();
  float humidity = dht11.readHumidity() * (100.0/93.0);
  /*
  my_serial.print("humidity ");
  my_serial.print(humidity);
  my_serial.print("\n");
  */

  // calculate and return pressure as hectoPascals
  //float pressure = sap.getPressure();

  FCM fcm = FCM(9, 10, A2);
  
  float pressure = (float)fcm.measure();
  /*
  my_serial.print("pressure? ");
  my_serial.print(pressure);
  my_serial.print("\n");
  */

  // calculate and return the illuminance as lux
  //float illuminance = sap.getIlluminance((float)analogRead(LDR_VOLTAGE_OUTPUT_PIN) / LDR_ADC_RESOLUTION * PWM_4_006V_AS_MILLIVOLTS);
  float illuminance =  LDR_4V_215_2R((float)analogRead(LDR_VOLTAGE_OUTPUT_PIN) / LDR_ADC_RESOLUTION * PWM_4_006V_AS_MILLIVOLTS);
  //float illuminance = ((float)analogRead(LDR_VOLTAGE_OUTPUT_PIN) / 1023.0f) * 5.0f;
  /*
  my_serial.print("illuminance ");
  my_serial.print(illuminance);
  my_serial.print("\n");
  */

  //digitalWrite(LED_BUILTIN, HIGH);

  for (int try_count = 8;;)
  {
    if (inp_http_post_measurements((const char*)INP_POST_SERVER_STUDENTS, (const char*)INP_POST_PATH_ADD_MEASUREMENT, station_id, temperature, humidity, pressure, illuminance))
      break;
    else
      --try_count;
  }

  //sap.begin(0, 102350);
  //digitalWrite(LED_BUILTIN, HIGH);
  
  //digitalWrite(LED_BUILTIN, LOW);

  /*
  for (int try_count = 8;;)
  {
     uint32_t interval;
     if (inp_http_ask_measurement_interval((const char*)INP_POST_SERVER_STUDENTS, (const char*)INP_POST_ASK_INTERVAL, station_id, &interval))
     {
      sleep_time = interval * MS_TIME_MIMUTE;
      break;
     }
     else
       --try_count;
  }
  */

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(9, INPUT);
  pinMode(10, INPUT);
  pinMode(11, INPUT);
  //digitalWrite(LED_BUILTIN, LOW);

  inp_sleep(sleep_time);
}
