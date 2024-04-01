#include <stdint.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

uint8_t crc8(size_t size, const uint8_t* data)
{
  uint8_t h = 0;
  for (const uint8_t* e = data + size; data != e; ++data)
  {
    h ^= *data;
    for (uint8_t c = 8; c--;)
      h = (h & 0x80) ? ((h << 1) ^ 7) : (h << 1);
  }
  return h;
}

#define HA_INDEX_TABLE 0x0
#define HA_INDEX_NETWORK_CONFIGURATION 0x1
#define HA_INDEX_FAN_SPEED 0x2
#define HA_INDEX_TEMPERATURE_MIN_LIMIT 0x3
#define HA_INDEX_TEMPERATURE_MAX_LIMIT 0x4
#define HA_WAIT 0x100
#define HA_CLOCK 0xD
#define HA_DATA 0xE

void haInitialize()
{
  pinMode(HA_CLOCK, OUTPUT);
  digitalWrite(HA_CLOCK, LOW);
  pinMode(HA_DATA, OUTPUT);
  digitalWrite(HA_DATA, LOW);
  delayMicroseconds(HA_WAIT);
  while (digitalRead(HA_DATA) == HIGH)
    delayMicroseconds(HA_WAIT);
  delayMicroseconds(HA_WAIT);
}

void haWrite(uint8_t variableIndex, uint8_t transferSize, const uint8_t* data)
{
  if (!transferSize)
    return;
  --transferSize;

  pinMode(HA_DATA, OUTPUT);
  digitalWrite(HA_DATA, LOW);
  pinMode(HA_DATA, INPUT);
  
  digitalWrite(HA_CLOCK, HIGH);
  while (digitalRead(HA_DATA) == LOW)
    delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, LOW);
  pinMode(HA_DATA, OUTPUT);
 
  digitalWrite(HA_DATA, LOW);// read write bit
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, HIGH);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, LOW);
  
  digitalWrite(HA_DATA, (variableIndex & 0x8) ? HIGH : LOW);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, HIGH);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, LOW);
  
  digitalWrite(HA_DATA, (variableIndex & 0x4) ? HIGH : LOW);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, HIGH);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, LOW);
  
  digitalWrite(HA_DATA, (variableIndex & 0x2) ? HIGH : LOW);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, HIGH);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, LOW);
  
  digitalWrite(HA_DATA, (variableIndex & 0x1) ? HIGH : LOW);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, HIGH);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, LOW);

  digitalWrite(HA_DATA, (transferSize & 0x8) ? HIGH : LOW);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, HIGH);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, LOW);
  
  digitalWrite(HA_DATA, (transferSize & 0x4) ? HIGH : LOW);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, HIGH);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, LOW);
  
  digitalWrite(HA_DATA, (transferSize & 0x2) ? HIGH : LOW);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, HIGH);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, LOW);
  
  digitalWrite(HA_DATA, (transferSize & 0x1) ? HIGH : LOW);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, HIGH);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, LOW);

  ++transferSize;

  for (const uint8_t* i = data, * e = i + transferSize; i != e; ++i)
  {
    uint8_t transferByte = *i;
    for (uint8_t bitCounter = 8; bitCounter--;)
    {
      digitalWrite(HA_DATA, (transferByte & (1 << bitCounter)) ? HIGH : LOW);
      delayMicroseconds(HA_WAIT);
      digitalWrite(HA_CLOCK, HIGH);
      delayMicroseconds(HA_WAIT);
      digitalWrite(HA_CLOCK, LOW);
    }
  }
  
  digitalWrite(HA_CLOCK, LOW);
  digitalWrite(HA_DATA, LOW);
  pinMode(HA_DATA, INPUT);
  delayMicroseconds(HA_WAIT);
}

void haRead(uint8_t variableIndex, uint8_t transferSize, uint8_t* data)
{
  if (!transferSize)
    return;
  --transferSize;
  
  pinMode(HA_DATA, OUTPUT);
  digitalWrite(HA_DATA, LOW);
  pinMode(HA_DATA, INPUT);
  
  digitalWrite(HA_CLOCK, HIGH);
  while (digitalRead(HA_DATA) == LOW)
    delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, LOW);
  pinMode(HA_DATA, OUTPUT);
 
  digitalWrite(HA_DATA, HIGH);// read write bit
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, HIGH);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, LOW);
  
  digitalWrite(HA_DATA, (variableIndex & 0x8) ? HIGH : LOW);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, HIGH);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, LOW);
  
  digitalWrite(HA_DATA, (variableIndex & 0x4) ? HIGH : LOW);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, HIGH);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, LOW);
  
  digitalWrite(HA_DATA, (variableIndex & 0x2) ? HIGH : LOW);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, HIGH);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, LOW);
  
  digitalWrite(HA_DATA, (variableIndex & 0x1) ? HIGH : LOW);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, HIGH);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, LOW);

  digitalWrite(HA_DATA, (transferSize & 0x8) ? HIGH : LOW);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, HIGH);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, LOW);
  
  digitalWrite(HA_DATA, (transferSize & 0x4) ? HIGH : LOW);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, HIGH);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, LOW);
  
  digitalWrite(HA_DATA, (transferSize & 0x2) ? HIGH : LOW);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, HIGH);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, LOW);
  
  digitalWrite(HA_DATA, (transferSize & 0x1) ? HIGH : LOW);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, HIGH);
  delayMicroseconds(HA_WAIT);
  digitalWrite(HA_CLOCK, LOW);

  ++transferSize;

  digitalWrite(HA_DATA, LOW);
  pinMode(HA_DATA, INPUT);

  for (uint8_t* i = data, * e = i + transferSize; i != e; ++i)
  {
    uint8_t transferByte = 0;
    for (uint8_t bitCounter = 8; bitCounter--;)
    {
      delayMicroseconds(HA_WAIT);
      digitalWrite(HA_CLOCK, HIGH);
      transferByte |= ((digitalRead(HA_DATA) == HIGH ? 1 : 0) << bitCounter);
      delayMicroseconds(HA_WAIT);
      digitalWrite(HA_CLOCK, LOW);
    }
    *i = transferByte;
  }
  
  digitalWrite(HA_CLOCK, LOW);
  delayMicroseconds(HA_WAIT);
}

#define NETWORK_CONFIGURATION_BUFFER_SIZE 0x80
char networkConfiguration[NETWORK_CONFIGURATION_BUFFER_SIZE];
#define WIFI_LOGIN_DATA_BUFFER_SIZE 0x80
char wifiLoginDataBuffer[WIFI_LOGIN_DATA_BUFFER_SIZE];
char* wifiSSID;
char* wifiPassword;
#define PACKET_DATA_BUFFER_SIZE 0x80
uint8_t packetDataBuffer[PACKET_DATA_BUFFER_SIZE];
WiFiUDP Udp;

void setup()
{
  pinMode(0, OUTPUT);
  digitalWrite(0, LOW);
  haInitialize();
  uint16_t tableLow[2];
  haRead(HA_INDEX_TABLE, 4, (uint8_t*)tableLow);
  uint16_t networkConfigurationOffset = tableLow[1];
  for (size_t destinationIncroment = 0; destinationIncroment != 0x80;)
  {
    haRead(HA_INDEX_NETWORK_CONFIGURATION, 16, (uint8_t*)networkConfiguration + destinationIncroment);
    destinationIncroment += 0x10;
    if (destinationIncroment != 0x80)
      tableLow[1] += 0x10;
    else
      tableLow[1] = networkConfigurationOffset;
    haWrite(HA_INDEX_TABLE, 4, (const uint8_t*)tableLow);
  }
  networkConfiguration[0x80 - 1] = 0;
  bool newNetworkConfiguration = false;
  char* wifiLogin = networkConfiguration;
  Serial.begin(9600);
  Serial.flush();
  while (wifiLogin)
  {
    size_t wifiLoginDataSize = 0;
    if (wifiLogin == wifiLoginDataBuffer)
    {
      Serial.print("Enter wifi SSID followed by pasword\n");
      wifiLoginDataSize = 0;
      while (!Serial.available())
        delay(0x10);
      while (wifiLoginDataSize != (WIFI_LOGIN_DATA_BUFFER_SIZE - 1) && Serial.available())
      {
        wifiLoginDataBuffer[wifiLoginDataSize++] = Serial.read();
        if (!Serial.available())
          delay(0x10);
      }
	    if (wifiLoginDataSize != 0x80)
		    wifiLoginDataBuffer[wifiLoginDataSize] = 0;
      for (uint8_t i = 0; i != 0x80; ++i)
        networkConfiguration[i] = wifiLoginDataBuffer[i];
      newNetworkConfiguration = true;
    }
    wifiLoginDataSize = 0;
    while (wifiLogin[wifiLoginDataSize])
        ++wifiLoginDataSize;
    bool mode2 = false;
    for (char* i = wifiLogin, * e = i + wifiLoginDataSize; !mode2 && i != e; ++i)
      if (*i == '"')
        mode2 = true;
    wifiLogin[wifiLoginDataSize++] = 0;
    if (mode2)
    {
      char* search = wifiLogin;
      while (search != wifiLogin + wifiLoginDataSize && *search != '"')
        ++search;
      if (search != wifiLogin + wifiLoginDataSize)
        *search++ = 0;
      wifiSSID = search != wifiLogin + wifiLoginDataSize ? search : 0;
      while (search != wifiLogin + wifiLoginDataSize && *search != '"')
        ++search;
      if (search != wifiLogin + wifiLoginDataSize)
        *search++ = 0;
      else
        wifiSSID = 0;
      while (search != wifiLogin + wifiLoginDataSize && *search != '"')
        ++search;
      if (search != wifiLogin + wifiLoginDataSize)
        *search++ = 0;
      wifiPassword = search != wifiLogin + wifiLoginDataSize ? search : 0;
      while (search != wifiLogin + wifiLoginDataSize && *search != '"')
        ++search;
      if (search != wifiLogin + wifiLoginDataSize)
        *search++ = 0;
      else
        wifiPassword = 0;
    }
    else
    {
      wifiSSID = wifiLogin;
      wifiPassword = wifiLogin;
      while (wifiPassword != wifiLogin + wifiLoginDataSize && *wifiPassword != ' ')
        ++wifiPassword;
      while (wifiPassword != wifiLogin + wifiLoginDataSize && *wifiPassword == ' ')
        *wifiPassword++ = 0;
      if (wifiPassword == wifiLogin + wifiLoginDataSize)
        wifiPassword = 0;
    }
    if (!wifiSSID)
      wifiSSID = "";
    if (!wifiPassword)
      wifiPassword = "";
    size_t wifiSSIDLength = 0;
    while (wifiSSID[wifiSSIDLength])
      ++wifiSSIDLength;
    bool SSIDExist = false;
    Serial.print("Scannig for \"");
    Serial.print(wifiSSID);
    Serial.print("\" network.\n");
    for (size_t SSIDCount = WiFi.scanNetworks(), SSIDIndex = 0; !SSIDExist && SSIDIndex != SSIDCount; ++SSIDIndex)
    {
      String someSSID = WiFi.SSID(SSIDIndex);
      size_t someSSIDLength = 0;
      while (someSSID[someSSIDLength])
        ++someSSIDLength;
      if (someSSIDLength == wifiSSIDLength)
      {
        size_t compareIndex = 0;
        while (compareIndex != wifiSSIDLength)
          if (someSSID[compareIndex] == wifiSSID[compareIndex])
            ++compareIndex;
          else
            break;
        if (compareIndex == wifiSSIDLength)
          SSIDExist = true;
      }
    }
    if (!SSIDExist)
    {
      if (wifiLogin == networkConfiguration)
      {
        wifiLogin = wifiLoginDataBuffer;
        Serial.print("Network not found.\n");
        continue;
      }
      else
        Serial.print("Warning SSID does not identify an existing network.\n.");
    }
    Serial.print("Connecting to network \"");
    Serial.print(wifiSSID);
    Serial.print("\" password \"");
    for (char* i = wifiPassword; *i; ++i)
      Serial.print('*');
    Serial.print("\".\n");
    WiFi.begin(wifiSSID, wifiPassword);
    for (unsigned int c = 0; WiFi.status() != WL_CONNECTED;)
    {
      digitalWrite(0, HIGH);
      delay(0x400);
      digitalWrite(0, LOW);
      delay(0x400);
      c = (c + 1) & 0x1F;
      if (!c && wifiLogin == networkConfiguration)
      {
        wifiLogin = wifiLoginDataBuffer;
        break;
      }
      Serial.print(!c ? "\n." : ".");
    }
    if (WiFi.status() == WL_CONNECTED)
      wifiLogin = 0;
  }
  if (newNetworkConfiguration)
  {
    haRead(HA_INDEX_TABLE, 4, (uint8_t*)tableLow);
    networkConfigurationOffset = tableLow[1];
    for (size_t destinationIncroment = 0; destinationIncroment != 0x80;)
    {
      haWrite(HA_INDEX_NETWORK_CONFIGURATION, 0x10, (uint8_t*)networkConfiguration + destinationIncroment);
      destinationIncroment += 0x10;
      if (destinationIncroment != 0x80)
        tableLow[1] += 0x10;
      else
        tableLow[1] = networkConfigurationOffset;
      haWrite(HA_INDEX_TABLE, 4, (const uint8_t*)tableLow);
    }
  }
  Udp.begin(0x29A);
  Serial.print(" connected\nNow listening at IP \"");
  Serial.print(WiFi.localIP().toString().c_str());
  Serial.print("\".\n");
  digitalWrite(0, LOW);
}

void loop()
{
  size_t packetSize = Udp.parsePacket();
  if (packetSize)
  {
    #ifdef DEBUG
    Serial.print("Received ");
    Serial.print(packetSize);
    Serial.print(" bytes from ");
    Serial.print(Udp.remoteIP().toString().c_str());
    Serial.print(", port ");
    Serial.print(Udp.remotePort());
    Serial.print(".\n");
    #endif
    size_t packetDataSize = Udp.read(packetDataBuffer, PACKET_DATA_BUFFER_SIZE);
    if (packetDataSize > 3 && packetDataBuffer[0] == 0x03 &&
      packetDataSize == (size_t)(3 + ((packetDataBuffer[1] >> 4) + 1)) &&
      packetDataBuffer[2 + ((packetDataBuffer[1] >> 4) + 1)] == crc8(2 + ((packetDataBuffer[1] >> 4) + 1), packetDataBuffer))
    {
      haWrite(packetDataBuffer[1] & 0x0F, (packetDataBuffer[1] >> 4) + 1, packetDataBuffer + 2);
      packetDataBuffer[0] = 0x02;
      packetDataBuffer[2] = crc8(((packetDataBuffer[1] >> 4) + 1), packetDataBuffer + 2);
      packetDataBuffer[3] = crc8(3, packetDataBuffer);
      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
      Udp.write(packetDataBuffer, 4);
      Udp.endPacket();
    }
    else if (packetDataSize == 3 && packetDataBuffer[0] == 0x01 && packetDataBuffer[2] == crc8(2, packetDataBuffer))
    {
      packetDataBuffer[0] = 0x00;
      haRead(packetDataBuffer[1] & 0x0F, (packetDataBuffer[1] >> 4) + 1, packetDataBuffer + 2);
      packetDataBuffer[2 + ((packetDataBuffer[1] >> 4) + 1)] = crc8(2 + ((packetDataBuffer[1] >> 4) + 1), packetDataBuffer);
      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
      Udp.write(packetDataBuffer, 3 + ((packetDataBuffer[1] >> 4) + 1));
      Udp.endPacket();
    }
    #ifdef DEBUG
    else
      Serial.print("Invalid packet.\n");
    #endif
  }
}
