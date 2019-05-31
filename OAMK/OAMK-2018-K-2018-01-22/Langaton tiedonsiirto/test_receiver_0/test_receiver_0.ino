//#include <SPI.h>
#include <RH_ASK.h>

RH_ASK driver;

void setup()
{
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  Serial.begin(9600);
  if (!driver.init())
    Serial.println("can't do");
}

uint8_t buffer[128];

void loop()
{
  uint8_t bufferLength = 127;
  if (driver.recv(buffer, &bufferLength))
  {
    digitalWrite(13, HIGH);
    buffer[bufferLength] = 0;
    Serial.println((const char*)buffer);
    delay(1000);
    digitalWrite(13, LOW);
  }
}


