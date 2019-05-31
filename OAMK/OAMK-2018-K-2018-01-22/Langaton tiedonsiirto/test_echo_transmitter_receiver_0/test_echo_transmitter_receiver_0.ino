//#include <SPI.h>
#include <RH_ASK.h>

RH_ASK driver;

void setup()
{
  if (!driver.init())
  {
    Serial.begin(9600);
    Serial.println("can't do");
  }
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
}

uint8_t buffer[128];

void loop()
{
  uint8_t messageLength = 128;
  if (driver.recv(buffer, &messageLength))
  {
    digitalWrite(13, HIGH);
    delay(1000);
    digitalWrite(13, LOW);
    driver.send((const uint8_t*)buffer, messageLength);
    driver.waitPacketSent();
  }
}


