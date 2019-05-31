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
  uint8_t messageLength = 0;
  while (!Serial.available())
    continue;
  while (messageLength != 128 && Serial.available())
  {
    buffer[messageLength++] = (uint8_t)Serial.read();
    if (!Serial.available())
      delay(16);
  }
  digitalWrite(13, HIGH);
  driver.send((const uint8_t*)buffer, messageLength);
  driver.waitPacketSent();
  delay(1000);
  digitalWrite(13, LOW);
}

