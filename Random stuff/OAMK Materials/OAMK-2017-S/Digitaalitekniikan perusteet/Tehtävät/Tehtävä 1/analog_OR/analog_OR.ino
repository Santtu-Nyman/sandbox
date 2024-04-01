
const int AnalogInPin0 = A0;
const int AnalogInPin1 = A1;
const int OutPin = 13;

int Sensor0Value = 0;
int Sensor1Value = 0;
int OutputValue = 0;

void setup() {
  Serial.begin(9600);
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
}

void loop()
{
  digitalWrite(13, analogRead(AnalogInPin0) > 512 || analogRead(AnalogInPin1) > 512 ? HIGH : LOW);
  delay(1000);
}
