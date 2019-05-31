/* Perus-porttipiiri AND Arduinolla toteutettuna

Digitaalitekniikan perusteet / perusharjoitus kurssin alkuvaiheisiin liittyen
23.8.2016
OAMK / Tekniikan yksikkö
-J.Kaski-

 */

// Määritellään output-napa siten, että Arduinon omakin LED näyttä HIGH-tilassa valoa. 
// Voit myös liittää 13-napaan sarjavastus (vähintään 250 ohmia; yli 10kohm LED ei enää syty)
// InPin1 ja InPin2-navat ovat nyt AND-portin sisäänmenoina ja ledPin sen ulostulo
const int outputQ0 =  A0;    // Datalehdellä jalka 3 (output-napa laskurilla, leutaan inputtina Arduinolla!)
const int outputQ1 =  A1;    // Datalehdellä jalka 2 (output-napa laskurilla, leutaan inputtina Arduinolla!)
const int outputQ2 =  A2;    // Datalehdellä jalka 6 (output-napa laskurilla, leutaan inputtina Arduinolla!)
const int outputQ3 =  A3;    // Datalehdellä jalka 7 (output-napa laskurilla, leutaan inputtina Arduinolla!)
const int CPU =  5;      // Datalehdellä jalka 5 (input, pulssitetaan tätä)
const int CPD =  4;      // Datalehdellä jalka 4 (input, pidetään ylhäällä)
const int TCU =  A4;      // Datalehdellä jalka 12 (output-napa laskurilla, leutaan inputtina Arduinolla!)
const int TCD =  A5;      // Datalehdellä jalka 13 (output-napa laskurilla, leutaan inputtina Arduinolla!)
// PL 5V:iin; MR maihin; D0 - D3: ei kytketä tässä esimerkissä; Q = output


// Alutetaan muuttujat:
int CPUin = LOW;             // Alustetaan tila 0V:iin
int CPDin = HIGH;            // Pidetään tämä ylhäällä jatkuvasti
int Q0 = 0;
int Q1 = 0;
int Q2 = 0;
int Q3 = 0;
int TCUval = 0;
int TCDval = 0;

int muutoksia = 0;

void setup() {
  // Asetetaan napojen roolit:
  pinMode(CPU, OUTPUT);
  pinMode(CPD, OUTPUT);
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);
}

void loop() {
  // Tämä loop-silmukka on se ajettava ohjelma, joka pyörii ympäri niin kauan kuin Arduinolla on virtaa.
  Q0 = analogRead(outputQ0);
  Q1 = analogRead(outputQ1);
  Q2 = analogRead(outputQ2);
  Q3 = analogRead(outputQ3);
  TCUval = analogRead(TCU);
  TCDval = analogRead(TCD);

  // Alaspäin laskeva kello ylätilaan ja pidetään siellä
  digitalWrite(CPD,HIGH);
  
  //Tuotetaan kellolle yksi jakson puoliska:
  if(CPUin == LOW){
  digitalWrite(CPU,HIGH);
  CPUin = HIGH;
  delay(1);
  }
  else{
  digitalWrite(CPU,LOW);
  CPUin = LOW;
  delay(1);
  muutoksia++;
  }

  //Tulostetaan ulostulot sarjaportille:
  Serial.print(muutoksia);
  Serial.print(" Q0 ");
  Serial.print(Q0);
  Serial.print(" Q1 ");
  Serial.print(Q1);
  Serial.print(" Q2 ");
  Serial.print(Q2);
  Serial.print(" Q3 ");
  Serial.print(Q3);
  Serial.print(" TCU ");
  Serial.print(TCUval);
  Serial.print(" TCD ");
  Serial.println(TCDval);
  delay(1000);
}

