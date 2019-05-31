unsigned int Index = 0;

void setup()
{
  Serial.begin(9600);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, INPUT);
  pinMode(13, OUTPUT);
}

void loop()
{
 digitalWrite(2, HIGH);
 digitalWrite(3, LOW);
 delay(16);
 digitalWrite(13, digitalRead(4));
 delay(16);
}

