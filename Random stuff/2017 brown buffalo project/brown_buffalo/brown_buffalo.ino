#include <stdint.h>
#include <math.h>
#include <EEPROM.h>

#define TM1637
#define GROVE_TEMPERATURE_SENSOR_VERSION_1_2

#define DEVICE_STATUS_LED_RED_PIN 2
#define DEVICE_STATUS_LED_GREEN_PIN 3
#define DEVICE_STATUS_LED_BLUE_PIN 4
#define LED_COLOR_RED 0x01
#define LED_COLOR_GREEN 0x02
#define LED_COLOR_BLUE 0x04

void initializeStatusLed()
{
  pinMode(DEVICE_STATUS_LED_RED_PIN, OUTPUT);
  digitalWrite(DEVICE_STATUS_LED_RED_PIN, HIGH);
  pinMode(DEVICE_STATUS_LED_GREEN_PIN, OUTPUT);
  digitalWrite(DEVICE_STATUS_LED_GREEN_PIN, HIGH);
  pinMode(DEVICE_STATUS_LED_BLUE_PIN, OUTPUT);
  digitalWrite(DEVICE_STATUS_LED_BLUE_PIN, HIGH);
}

void setStatusLedColor(uint8_t color)
{
  digitalWrite(DEVICE_STATUS_LED_RED_PIN, (color & 0x01) ? LOW : HIGH);
  digitalWrite(DEVICE_STATUS_LED_GREEN_PIN, (color & 0x02) ? LOW : HIGH);
  digitalWrite(DEVICE_STATUS_LED_BLUE_PIN, (color & 0x04) ? LOW : HIGH);
}

#ifdef GROVE_TEMPERATURE_SENSOR_VERSION_1_2
#define GROVE_TEMPERATURE_SENSOR_READ_PIN A0
void initializeTemperatureSensor()
{
  pinMode(GROVE_TEMPERATURE_SENSOR_READ_PIN, INPUT);
  delayMicroseconds(3);
}

float readTemperatureSensor()
{
  return (1.0f / ((log((1023.0f / (float)analogRead(GROVE_TEMPERATURE_SENSOR_READ_PIN)) - 1.0f) * 0.00023391812f) + 0.00335401643f)) - 273.15f;
}
#endif

#ifdef TM1637
#define TM1637_DIO_PIN 8
#define TM1637_CLK_PIN 9
#define DISPLAY_TEXT_LEGTH 4
void setDisplayText(const uint8_t* text)
{
  const uint8_t bitmaps[16] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71 };
  const uint8_t TM1637AddressAuto = 0x40;
  const uint8_t TM1637Address = 0xC0;
  const uint8_t TM1637DisplayControl = 0x8A;
  const uint8_t bufferSizes[3] = { 1, 5, 1 };
  uint8_t commandBuffer[5] = { TM1637Address, text[0], text[1], text[2], text[3] };
  for (uint8_t* i = commandBuffer + 1, * e = i + 4, c; i != e; ++i)
  {
    c = *i;
    if (c > 0x60)
      c -= 0x57;
    else if (c > 0x40)
      c -= 0x37;
    else if (c > 0x30)
      c -= 0x30;
    *i = (c != 0x20) ? c != 0x2D ? bitmaps[c & 0xF] : 0x40 : 0;
  }
  uint8_t* buffers[3] = { (uint8_t*)&TM1637AddressAuto, commandBuffer, (uint8_t*)&TM1637DisplayControl };
  for (uint8_t bufferIndex = 0; bufferIndex != 3; ++bufferIndex)
    for (uint8_t commandNotSend = 1, commandSendError = 0; commandNotSend;)
    {
      pinMode(TM1637_DIO_PIN, OUTPUT);
      delayMicroseconds(3);
      digitalWrite(TM1637_CLK_PIN, HIGH);
      digitalWrite(TM1637_DIO_PIN, HIGH);
      delayMicroseconds(3);
      digitalWrite(TM1637_DIO_PIN, LOW);
      delayMicroseconds(3);
      digitalWrite(TM1637_CLK_PIN, LOW);
      delayMicroseconds(3);
      for (uint8_t* data = buffers[bufferIndex], * dataEnd = data + bufferSizes[bufferIndex]; !commandSendError && data != dataEnd; ++data)
      {
        uint8_t currentByte = *data;
        for (int i = 8; i--;)
        {
          digitalWrite(TM1637_CLK_PIN, LOW);
          digitalWrite(TM1637_DIO_PIN, (currentByte & 1) ? HIGH : LOW);
          currentByte >>= 1;
          delayMicroseconds(3);
          digitalWrite(TM1637_CLK_PIN, HIGH);
          delayMicroseconds(3);
        }
        digitalWrite(TM1637_CLK_PIN, LOW);
        pinMode(TM1637_DIO_PIN, INPUT);
        delayMicroseconds(3);
        if (digitalRead(TM1637_DIO_PIN) == LOW)
        {
          digitalWrite(TM1637_CLK_PIN, HIGH);
          delayMicroseconds(3);
          digitalWrite(TM1637_CLK_PIN, LOW);
          pinMode(TM1637_DIO_PIN, OUTPUT);
        }
        else
          commandSendError = 1;
      }
      if (!commandSendError)
      {
        digitalWrite(TM1637_CLK_PIN, LOW);
        digitalWrite(TM1637_DIO_PIN, LOW);
        delayMicroseconds(3);
        digitalWrite(TM1637_CLK_PIN, HIGH);
        delayMicroseconds(3);
        digitalWrite(TM1637_DIO_PIN, HIGH);
        delayMicroseconds(3);
        commandNotSend = 0;
      }
      else
      {
        delay(100);
        pinMode(TM1637_DIO_PIN, OUTPUT);
        digitalWrite(TM1637_CLK_PIN, HIGH);
        digitalWrite(TM1637_DIO_PIN, HIGH);
        delay(100);
        commandSendError = 0;
      }
    }
}

void initializeDisplay()
{
  uint8_t empty[DISPLAY_TEXT_LEGTH];
  for (uint8_t* i = empty, * e = i + DISPLAY_TEXT_LEGTH; i != e; ++i)
    *i = 0x20;
  pinMode(TM1637_DIO_PIN, OUTPUT);
  digitalWrite(TM1637_DIO_PIN, LOW);
  pinMode(TM1637_CLK_PIN, OUTPUT);
  digitalWrite(TM1637_CLK_PIN, LOW);
  delayMicroseconds(3);
  setDisplayText(empty);
}
#endif

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

#define FAN_CONTROL_RELAY_PIN_0 5
#define FAN_CONTROL_RELAY_PIN_1 6
#define FAN_CONTROL_RELAY_PIN_2 7
void initializeFan()
{
  pinMode(FAN_CONTROL_RELAY_PIN_0, OUTPUT);
  digitalWrite(FAN_CONTROL_RELAY_PIN_0, LOW);
  pinMode(FAN_CONTROL_RELAY_PIN_1, OUTPUT);
  digitalWrite(FAN_CONTROL_RELAY_PIN_1, LOW);
  pinMode(FAN_CONTROL_RELAY_PIN_2, OUTPUT);
  digitalWrite(FAN_CONTROL_RELAY_PIN_2, LOW);
}

void setFanSpeed(uint8_t speed)
{
  static uint8_t currentSpeed = 0;
  if (speed == currentSpeed)
    return;
  currentSpeed = speed;
  speed = ~(0xFF << speed);
  digitalWrite(FAN_CONTROL_RELAY_PIN_0, (speed & 1) ? HIGH : LOW);
  digitalWrite(FAN_CONTROL_RELAY_PIN_1, (speed & 2) ? HIGH : LOW);
  digitalWrite(FAN_CONTROL_RELAY_PIN_2, (speed & 4) ? HIGH : LOW);
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

void printCelsius(uint8_t* buffer, float value)
{
  uint8_t negative = value < 0.0f ? 0x2D : 0x20;
  uint8_t integer = (uint8_t)(negative == 0x20 ? value : -value);
  buffer[1] = 0x30 + ((integer / 10) % 10);
  buffer[2] = 0x30 + (integer % 10);
  buffer[3] = 0x43;
  for (uint8_t* i = buffer + 4, * e = buffer + DISPLAY_TEXT_LEGTH; i != e; ++i)
    *i = 0x20;
}

typedef struct deviceControlStructure
{
  float temperature;
  float minTemperatureLimit;
  float maxTemperatureLimit;
  uint8_t fanSpeed;
  uint8_t fanOn;
  uint8_t displayText[DISPLAY_TEXT_LEGTH];
  void* remoteAccessTable[0x10];
  uint8_t networkConfiguration[0x80];
} deviceControlStructure;

deviceControlStructure deviceControl;

void setup()
{
  #ifdef DEBUG
  Serial.begin(9600);
  Serial.print("Running...\n");
  #endif
  ahInitialize();
  initializeFan();
  initializeStatusLed();
  initializeDisplay();
  initializeTemperatureSensor();
  deviceControl.temperature = readTemperatureSensor();
  deviceControl.minTemperatureLimit = 26.0f;
  deviceControl.maxTemperatureLimit = 28.0f;
  deviceControl.fanSpeed = 1;
  deviceControl.fanOn = 0;
  deviceControl.remoteAccessTable[AH_INDEX_TABLE] = deviceControl.remoteAccessTable;
  deviceControl.remoteAccessTable[AH_INDEX_NETWORK_CONFIGURATION] = deviceControl.networkConfiguration;
  deviceControl.remoteAccessTable[AH_INDEX_FAN_SPEED] = &deviceControl.fanSpeed;
  deviceControl.remoteAccessTable[AH_INDEX_TEMPERATURE_MIN_LIMIT] = &deviceControl.minTemperatureLimit;
  deviceControl.remoteAccessTable[AH_INDEX_TEMPERATURE_MAX_LIMIT] = &deviceControl.maxTemperatureLimit;
  deviceControl.remoteAccessTable[AH_INDEX_TEMPERATURE] = &deviceControl.temperature;
  for (uint8_t* i = deviceControl.displayText, * e = deviceControl.displayText + DISPLAY_TEXT_LEGTH; i != e; ++i)
    *i = 0x20;
  for (void** i = deviceControl.remoteAccessTable + 0x06, ** e = deviceControl.remoteAccessTable + 0x10; i != e; ++i)
    *i = 0;
  loadNetworkConfiguration(deviceControl.networkConfiguration);
  setStatusLedColor(LED_COLOR_GREEN);
}

void loop()
{
  deviceControl.temperature = readTemperatureSensor();
  uint8_t remoteWriteIndex = ahRunService(deviceControl.remoteAccessTable);
  if (remoteWriteIndex == AH_INDEX_NETWORK_CONFIGURATION)
    storeNetworkConfiguration(deviceControl.networkConfiguration);
  if (deviceControl.fanSpeed) {
    printCelsius(deviceControl.displayText, deviceControl.temperature);
    setDisplayText(deviceControl.displayText);
    if (deviceControl.fanOn && deviceControl.temperature < deviceControl.minTemperatureLimit)
      deviceControl.fanOn = 0;
    else if (!deviceControl.fanOn && deviceControl.temperature > deviceControl.maxTemperatureLimit)
      deviceControl.fanOn = 1;
    if (deviceControl.fanOn) {
      setStatusLedColor(LED_COLOR_BLUE);
      setFanSpeed(deviceControl.fanSpeed);
	}
    else {
      setStatusLedColor(LED_COLOR_GREEN);
      setFanSpeed(0);
    }
  }
  else {
    setStatusLedColor(LED_COLOR_RED);
    setFanSpeed(0);
    for (uint8_t* i = deviceControl.displayText, * e = deviceControl.displayText + DISPLAY_TEXT_LEGTH; i != e; ++i)
      *i = 0x20;
    setDisplayText(deviceControl.displayText);
  }
  #ifdef DEBUG
  Serial.print("speed ");
  Serial.print(deviceControl.fanSpeed);
  Serial.print(" on ");
  Serial.print(deviceControl.fanOn);
  Serial.print(" temperature ");
  Serial.print(deviceControl.temperature);
  Serial.print(" min ");
  Serial.print(deviceControl.minTemperatureLimit);
  Serial.print(" max ");
  Serial.print(deviceControl.maxTemperatureLimit);
  Serial.print(".\n");
  #endif
  delay(0x10);
}









