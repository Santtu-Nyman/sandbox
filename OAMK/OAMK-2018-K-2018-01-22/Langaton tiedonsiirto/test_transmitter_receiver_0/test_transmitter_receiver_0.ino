//#include <SPI.h>
#include <RH_ASK.h>

RH_ASK driver;

void setup()
{
  Serial.begin(9600);
  if (!driver.init())
    Serial.println("can't do");
}

uint8_t buffer[128];

void loop()
{
  uint8_t messageLength;
  if (Serial.available())
  {
    messageLength = 0;
    while (messageLength != 128 && Serial.available())
    {
      buffer[messageLength++] = (uint8_t)Serial.read();
      if (!Serial.available())
        delay(16);
    }
    driver.send((const uint8_t*)buffer, messageLength);
    driver.waitPacketSent();
  }
  else
  {
    messageLength = 127;
    if (driver.recv(buffer, &messageLength))
    {
      buffer[messageLength] = 0;
      Serial.println((const char*)buffer);
    }
  }
}


