/*
  Cool Water Dispenser fast temperature measurement pragram for Arduino version 1.0.0 2019-03-23 written by Santtu Nyman.
  git repository https://github.com/AP-Elektronica-ICT/ip2019-coolwater
  
  Description
    Cool Water Dispenser fast temperature measurement pragram for Arduino.
    Extremely simple program for calibrating printed temperature sensor.
    
  Version history
    version 1.0.0 2019-03-23
      First version.
*/

#include <arduino.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#include "mcp3201.h"

float read_sd18b20()
{
  OneWire one_wire(5); 
  DallasTemperature dallas(&one_wire);
  dallas.requestTemperatures();
  return dallas.getTempCByIndex(0);
}

#define SPI_CS 2
#define SPI_CLK 3
#define SPI_DOUT 4

void setup()
{
  Serial.begin(9600);
}

int rv()
{
  int m[9];
  for (int i = 0; i != (sizeof(m) / sizeof(int)); ++i)
  {
    m[i] = cwd_mcp3201_read(SPI_CS, SPI_CLK, SPI_DOUT);
    delay(16);
  }
  for (int i = 1; i != (sizeof(m) / sizeof(int));)
    if (m[i - 1] > m[i])
    {
      int tmp = m[i];
      m[i] = m[i - 1];
      m[i - 1] = tmp;
      if (i != 1)
        --i;
    }
    else
      ++i;
  return m[(sizeof(m) / sizeof(int)) / 2];
}

void loop()
{
  int x = rv();
  float y = read_sd18b20();
  Serial.print(x);
  Serial.print(' ');
  Serial.print(y);
  Serial.print('\n');
  delay(1000);
}
