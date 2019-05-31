const int SensorDataLength = 16;
int SensorValues[SensorDataLength] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int SensorNormal = 0;
int SensorErrorTolarance = 1;
int SensorIndex = 0;
int Total = 0;
int OutputValue = 0;

void setup()
{
  for (int* SensorData = SensorValues, * SensorDataEnd = SensorValues + SensorDataLength; SensorData != SensorDataEnd; ++SensorData)
   *SensorData = 0;
  Serial.begin(9600);
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  delay(16);
  SensorNormal = 0;
  int SensorConfiguratinData[4] = { 0, 0, 0, 0 };
  for (int DataIndex = 0; DataIndex != 4; ++DataIndex)
  {
    for (int Count = 64; Count; --Count)
    {
      SensorConfiguratinData[DataIndex] += analogRead(A0);
      delay(16);
    }
    SensorConfiguratinData[DataIndex] /= 64;
    SensorNormal += SensorConfiguratinData[DataIndex];
  }
  SensorNormal /= 4;
  for (int* SensorData = SensorValues, * SensorDataEnd = SensorValues + SensorDataLength; SensorData != SensorDataEnd; ++SensorData)
   *SensorData = SensorNormal;
  for (int Count = 64; Count; --Count)
  {
    delay(16);
    SensorValues[SensorIndex++ & (SensorDataLength - 1)] = analogRead(A0);
    Total = 0;
    for (int* SensorData = SensorValues, * SensorDataEnd = SensorValues + SensorDataLength; SensorData != SensorDataEnd; ++SensorData)
      Total += *SensorData;
     Total /= SensorDataLength;
    int New = Total < SensorNormal ? SensorNormal - Total : Total - SensorNormal;
    New += 1;
    if (New > SensorErrorTolarance)
     SensorErrorTolarance = New;
  }
  
  //Serial.print("Sensor normal value ");
  //Serial.println(SensorNormal);
  //delay(5000);
}

void loop()
{
  SensorValues[SensorIndex++ & (SensorDataLength - 1)] = analogRead(A0);
  Total = 0;
  for (int* SensorData = SensorValues, * SensorDataEnd = SensorValues + SensorDataLength; SensorData != SensorDataEnd; ++SensorData)
   Total += *SensorData;
  Total /= SensorDataLength;
  Serial.println(Total);
  if (Total > SensorNormal + SensorErrorTolarance || Total < SensorNormal - SensorErrorTolarance)
    OutputValue = 1;
  else
    OutputValue = 0;
   //Serial.println(OutputValue);
  if (OutputValue == 1)
      digitalWrite(13, HIGH);
  else
      digitalWrite(13, LOW);
  delay(16);
}

