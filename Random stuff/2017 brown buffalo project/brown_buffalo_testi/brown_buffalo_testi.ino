#include <stdint.h>
#include <EEPROM.h>

#define AH_INDEX_TABLE 0x0
#define AH_INDEX_NETWORK_CONFIGURATION 0x1
#define AH_INDEX_FAN_SPEED 0x2
#define AH_INDEX_TEMPERATURE_MIN_LIMIT 0x3
#define AH_INDEX_TEMPERATURE_MAX_LIMIT 0x4
#define AH_INDEX_TEMPERATURE 0x5
#define AH_CLOCK A1
#define AH_DATA A2
#define AH_TIMEOUT 0x400
#define AH_WRITE_HIGH 675
#define AH_WAIT 0x100

void ahInitialize()
{
  pinMode(AH_CLOCK, OUTPUT);
  analogWrite(AH_CLOCK, 0);
  pinMode(AH_CLOCK, INPUT);
  pinMode(AH_DATA, OUTPUT);
  analogWrite(AH_DATA, 0);
  pinMode(AH_DATA, INPUT);
  delayMicroseconds(AH_WAIT);
}

uint8_t ahRunService(void** table)
{
  pinMode(AH_CLOCK, INPUT);
  if (analogRead(AH_CLOCK) & 0x300)
  {
    unsigned long timeout = millis() + AH_TIMEOUT;
    pinMode(AH_DATA, OUTPUT);
    analogWrite(AH_DATA, AH_WRITE_HIGH);
    while (analogRead(AH_CLOCK) & 0x300)
      if (millis() > timeout)
      {
        pinMode(AH_CLOCK, OUTPUT);
        analogWrite(AH_CLOCK, 0);
        pinMode(AH_CLOCK, INPUT);
        analogWrite(AH_DATA, 0);
        return 0x10;
      }
    timeout = millis() + AH_TIMEOUT;
    analogWrite(AH_DATA, 0);
    pinMode(AH_DATA, INPUT);
    while (!(analogRead(AH_CLOCK) & 0x300))
      if (millis() > timeout)
      {
        pinMode(AH_DATA, OUTPUT);
        analogWrite(AH_DATA, 0);
        return 0x10;
      }
    timeout = millis() + AH_TIMEOUT;
    uint8_t readMode = ((analogRead(AH_DATA) & 0x300) ? 1 : 0);
    while (analogRead(AH_CLOCK) & 0x300)
      if (millis() > timeout)
      {
        pinMode(AH_CLOCK, OUTPUT);
        analogWrite(AH_CLOCK, 0);
        pinMode(AH_CLOCK, INPUT);
        pinMode(AH_DATA, OUTPUT);
        analogWrite(AH_DATA, 0);
        return 0x10;
      }
    timeout = millis() + AH_TIMEOUT;
    while (!(analogRead(AH_CLOCK) & 0x300))
      if (millis() > timeout)
      {
        pinMode(AH_DATA, OUTPUT);
        analogWrite(AH_DATA, 0);
        return 0x10;
      }
    timeout = millis() + AH_TIMEOUT;
    uint8_t variableIndex = ((analogRead(AH_DATA) & 0x300) ? 1 : 0) << 3;
    while (analogRead(AH_CLOCK) & 0x300)
      if (millis() > timeout)
      {
        pinMode(AH_CLOCK, OUTPUT);
        analogWrite(AH_CLOCK, 0);
        pinMode(AH_CLOCK, INPUT);
        pinMode(AH_DATA, OUTPUT);
        analogWrite(AH_DATA, 0);
        return 0x10;
      }
    timeout = millis() + AH_TIMEOUT;
    while (!(analogRead(AH_CLOCK) & 0x300))
      if (millis() > timeout)
      {
        pinMode(AH_DATA, OUTPUT);
        analogWrite(AH_DATA, 0);
        return 0x10;
      }
    timeout = millis() + AH_TIMEOUT;
    variableIndex |= ((analogRead(AH_DATA) & 0x300) ? 1 : 0) << 2;
    while (analogRead(AH_CLOCK) & 0x300)
      if (millis() > timeout)
      {
        pinMode(AH_CLOCK, OUTPUT);
        analogWrite(AH_CLOCK, 0);
        pinMode(AH_CLOCK, INPUT);
        pinMode(AH_DATA, OUTPUT);
        analogWrite(AH_DATA, 0);
        return 0x10;
      }
    timeout = millis() + AH_TIMEOUT;
    while (!(analogRead(AH_CLOCK) & 0x300))
      if (millis() > timeout)
      {
        pinMode(AH_DATA, OUTPUT);
        analogWrite(AH_DATA, 0);
        return 0x10;
      }
    timeout = millis() + AH_TIMEOUT;
    variableIndex |= ((analogRead(AH_DATA) & 0x300) ? 1 : 0) << 1;
    while (analogRead(AH_CLOCK) & 0x300)
      if (millis() > timeout)
      {
        pinMode(AH_CLOCK, OUTPUT);
        analogWrite(AH_CLOCK, 0);
        pinMode(AH_CLOCK, INPUT);
        pinMode(AH_DATA, OUTPUT);
        analogWrite(AH_DATA, 0);
        return 0x10;
      }
    timeout = millis() + AH_TIMEOUT;
    while (!(analogRead(AH_CLOCK) & 0x300))
      if (millis() > timeout)
      {
        pinMode(AH_DATA, OUTPUT);
        analogWrite(AH_DATA, 0);
        return 0x10;
      }
    timeout = millis() + AH_TIMEOUT;
    variableIndex |= ((analogRead(AH_DATA) & 0x300) ? 1 : 0);
    while (analogRead(AH_CLOCK) & 0x300)
      if (millis() > timeout)
      {
        pinMode(AH_CLOCK, OUTPUT);
        analogWrite(AH_CLOCK, 0);
        pinMode(AH_CLOCK, INPUT);
        pinMode(AH_DATA, OUTPUT);
        analogWrite(AH_DATA, 0);
        return 0x10;
      }
    timeout = millis() + AH_TIMEOUT;
    while (!(analogRead(AH_CLOCK) & 0x300))
      if (millis() > timeout)
      {
        pinMode(AH_DATA, OUTPUT);
        analogWrite(AH_DATA, 0);
        return 0x10;
      }
    timeout = millis() + AH_TIMEOUT;
    uint8_t transferSize = ((analogRead(AH_DATA) & 0x300) ? 1 : 0) << 3;
    while (analogRead(AH_CLOCK) & 0x300)
      if (millis() > timeout)
      {
        pinMode(AH_CLOCK, OUTPUT);
        analogWrite(AH_CLOCK, 0);
        pinMode(AH_CLOCK, INPUT);
        pinMode(AH_DATA, OUTPUT);
        analogWrite(AH_DATA, 0);
        return 0x10;
      }
    timeout = millis() + AH_TIMEOUT;
    while (!(analogRead(AH_CLOCK) & 0x300))
      if (millis() > timeout)
      {
        pinMode(AH_DATA, OUTPUT);
        analogWrite(AH_DATA, 0);
        return 0x10;
      }
    timeout = millis() + AH_TIMEOUT;
    transferSize |= ((analogRead(AH_DATA) & 0x300) ? 1 : 0) << 2;
    while (analogRead(AH_CLOCK) & 0x300)
      if (millis() > timeout)
      {
        pinMode(AH_CLOCK, OUTPUT);
        analogWrite(AH_CLOCK, 0);
        pinMode(AH_CLOCK, INPUT);
        pinMode(AH_DATA, OUTPUT);
        analogWrite(AH_DATA, 0);
        return 0x10;
      }
    timeout = millis() + AH_TIMEOUT;
    while (!(analogRead(AH_CLOCK) & 0x300))
      if (millis() > timeout)
      {
        pinMode(AH_DATA, OUTPUT);
        analogWrite(AH_DATA, 0);
        return 0x10;
      }
    timeout = millis() + AH_TIMEOUT;
    transferSize |= ((analogRead(AH_DATA) & 0x300) ? 1 : 0) << 1;
    while (analogRead(AH_CLOCK) & 0x300)
      if (millis() > timeout)
      {
        pinMode(AH_CLOCK, OUTPUT);
        analogWrite(AH_CLOCK, 0);
        pinMode(AH_CLOCK, INPUT);
        pinMode(AH_DATA, OUTPUT);
        analogWrite(AH_DATA, 0);
        return 0x10;
      }
    timeout = millis() + AH_TIMEOUT;
    while (!(analogRead(AH_CLOCK) & 0x300))
      if (millis() > timeout)
      {
        pinMode(AH_DATA, OUTPUT);
        analogWrite(AH_DATA, 0);
        return 0x10;
      }
    transferSize |= ((analogRead(AH_DATA) & 0x300) ? 1 : 0);
    ++transferSize;
    timeout = millis() + AH_TIMEOUT;
    uint8_t* data = (uint8_t*)table[variableIndex];
    uint8_t* dataEnd = data + transferSize;
    if (readMode)
    {
      pinMode(AH_DATA, OUTPUT);
      while (data != dataEnd)
      {
        uint8_t transferByte = *data++;
        for (uint8_t bitCounter = 8; bitCounter--;)
        {
          analogWrite(AH_DATA, (transferByte & (1 << bitCounter)) ? AH_WRITE_HIGH : 0);
          while (analogRead(AH_CLOCK) & 0x300)
            if (millis() > timeout)
            {
              pinMode(AH_CLOCK, OUTPUT);
              analogWrite(AH_CLOCK, 0);
              pinMode(AH_CLOCK, INPUT);
              analogWrite(AH_DATA, 0);
              return 0x10;
            }
          timeout = millis() + AH_TIMEOUT;
          while (!(analogRead(AH_CLOCK) & 0x300))
            if (millis() > timeout)
            {
              analogWrite(AH_DATA, 0);
              return 0x10;
            }
          timeout = millis() + AH_TIMEOUT;
        }
      }
      analogWrite(AH_DATA, 0);
      while (analogRead(AH_CLOCK) & 0x300)
        if (millis() > timeout)
        {
          pinMode(AH_CLOCK, OUTPUT);
          analogWrite(AH_CLOCK, 0);
          pinMode(AH_CLOCK, INPUT);
          analogWrite(AH_DATA, 0);
          return 0x10;
        }
      return 0x10;
    }
    else
    {
      while (data != dataEnd)
      {
        uint8_t transferByte = 0;
        for (uint8_t bitCounter = 8; bitCounter--;)
        {
          while (analogRead(AH_CLOCK) & 0x300)
            if (millis() > timeout)
            {
              pinMode(AH_CLOCK, OUTPUT);
              analogWrite(AH_CLOCK, 0);
              pinMode(AH_CLOCK, INPUT);
              pinMode(AH_DATA, OUTPUT);
              analogWrite(AH_DATA, 0);
              return 0x10;
            }
          timeout = millis() + AH_TIMEOUT;
          while (!(analogRead(AH_CLOCK) & 0x300))
            if (millis() > timeout)
            {
              pinMode(AH_DATA, OUTPUT);
              analogWrite(AH_DATA, 0);
              return 0x10;
            }
          transferByte |= (((analogRead(AH_DATA) & 0x300) ? 1 : 0) << bitCounter);
          timeout = millis() + AH_TIMEOUT;
        }
        *data++ = transferByte;
      }
      while (analogRead(AH_CLOCK) & 0x300)
        if (millis() > timeout)
        {
          pinMode(AH_CLOCK, OUTPUT);
          analogWrite(AH_CLOCK, 0);
          pinMode(AH_CLOCK, INPUT);
          pinMode(AH_DATA, OUTPUT);
          analogWrite(AH_DATA, 0);
          return 0x10;
        }
      pinMode(AH_DATA, OUTPUT);
      analogWrite(AH_DATA, 0);
      return variableIndex;
    }
  }
  else
    return 0x10;
}

static uint8_t networkConfurationBufferIsLoaded;
static uint8_t networkConfurationBuffer[0x80];
void loadNetworkConfiguration(uint8_t* buffer)
{
  if (!networkConfurationBufferIsLoaded)
  {
    networkConfurationBufferIsLoaded = 1;
    for (uint8_t i = 0; i != 0x80; ++i)
      networkConfurationBuffer[i] = (uint8_t)EEPROM.read(i);
  }
  for (uint8_t i = 0; i != 0x80; ++i)
    buffer[i] = networkConfurationBuffer[i];
}

void storeNetworkConfiguration(const uint8_t* buffer)
{
  if (!networkConfurationBufferIsLoaded)
  {
    networkConfurationBufferIsLoaded = 1;
    for (uint8_t i = 0; i != 0x80; ++i)
     networkConfurationBuffer[i] = (uint8_t)EEPROM.read(i);
  }
  for (uint8_t i = 0; i != 0x80; ++i)
    if (buffer[i] != networkConfurationBuffer[i])
    {
      networkConfurationBuffer[i] = buffer[i];
      EEPROM.write(i, networkConfurationBuffer[i]);
    }
}

void* ahTable[0x10];

uint8_t networkConfuration[0x80];
uint8_t fanSpeed = 0;
float minLimit = 0.0f;
float maxLimit = 0.0f;
float temperature = 0.0f;

void setup()
{
  // laitetaan arduinon ja huzzahin välinen kummunikaatio linja käyttä valmiiksi
  ahInitialize();
  loadNetworkConfiguration(networkConfuration);
  // täytetään taulukko ennen kuin kutsutaan ahRunService
  ahTable[AH_INDEX_TABLE] = ahTable;
  ahTable[AH_INDEX_NETWORK_CONFIGURATION] = networkConfuration;
  ahTable[AH_INDEX_FAN_SPEED] = &fanSpeed;
  ahTable[AH_INDEX_TEMPERATURE_MIN_LIMIT] = &minLimit;
  ahTable[AH_INDEX_TEMPERATURE_MAX_LIMIT] = &maxLimit;
  ahTable[AH_INDEX_TEMPERATURE] = &temperature;
}

void loop()
{
  if (ahRunService(ahTable) == AH_INDEX_NETWORK_CONFIGURATION)
    storeNetworkConfiguration(networkConfuration);
  delay(16);
}












