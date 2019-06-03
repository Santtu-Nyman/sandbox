/*
  MsTimer2 is a small and very easy to use library to interface Timer2 with
  humans. It's called MsTimer2 because it "hardcodes" a resolution of 1
  millisecond on timer2
  For Details see: http://www.arduino.cc/playground/Main/MsTimer2
  
  Lisäys / JMK:
  Tämä ohjelma tuottaa kahdella kanavalla kanttipulssia. Digitaalinapa 3 on tuplataajuudella napaan
  2 verrattuna. Taajuuden voi määritellä alun muuttujissa pulssin kestoajan mukaan, joka on puolet
  koko jaksonajasta.
  Tämä ohjelma lukee kolmea kanavaa A0, A1 ja A2, joista A0:n sensoriarvo nostetaan 3000:lla
  ja A1 nostetaan 1500:lla tulostuksellisen päällekkäisyyden välttämiseksi. Kanavat luetaan sensori-
  arvoina 0 - 1023, vastaten jännitettä 0 - 5V.
*/
#include <MsTimer2.h>
volatile unsigned long aika = 0; //volatile-muuttuja on globaali muuttujatyyppi
volatile int sensorValue0 = 0; //Saatavilla maksimissaan 6 mittauskanavaa, nyt otetaan 3.
volatile int sensorValue1 = 0;
volatile int sensorValue2 = 0;
const int analogInPin0 = A0; //Määritellään mittaavat navat.
const int analogInPin1 = A1;
const int analogInPin2 = A2;
int naytevali_ts = 20; //Näyteväli(ms), säädä sopivaksi, 2ms taitaa olla minimi keskeytyksellä.
const int OutPin2 = 2; //Jännitesyötön navan numero Arduino UNO-boardilla.
const int OutPin3 = 3;
int OutPulseLength = 2000; // Hitaamman kanttipulssin (napa 2) jaksonaika = 2 * tässä annettu ms-arvo.
int viive = 0; // Apumuuttuja viiveiden laskemiseksi.
int Tila = 0; // Syöttötilan määrittelyssä käytetty apumuuttuja. Vaihtelee ajastetusti ohjelmassa.
volatile int tulostaAika = 1; //Aikaleiman tulostus. 0: ei tulosta; 1: tulostaa aikaleiman (ms).

// Keskeytys vie tänne
void flash()
{
  aika = millis(); //Aikaleima dataloggerille
  sensorValue0 = analogRead(analogInPin0); //Sensoriarvon luku
  sensorValue0 = sensorValue0 + 3000; // Viedään käyrät eri tasolle, eka ylimpänä 
  sensorValue1 = analogRead(analogInPin1);
  sensorValue1 = sensorValue1 + 1500;
  sensorValue2 = analogRead(analogInPin2);
  if(tulostaAika == 1){
  Serial.print(aika); //Jos tulostus-optio on päällä niin aikaleima mukaan.
  Serial.print("\t ");
  }
  Serial.print(sensorValue0);
  Serial.print("\t ");
  Serial.print(sensorValue1);
  Serial.print("\t ");
  Serial.println(sensorValue2);
}

void setup()
{
  Serial.begin(9600); //Tätä voi nopeuttaa jos on tarvis
  pinMode(OutPin2, OUTPUT); //Määritellään output-navat
  pinMode(OutPin3, OUTPUT);
  
  MsTimer2::set(naytevali_ts, flash); //Ajastimen alustus; näyteväli annetaan millisekunteina määrittelyissä.
  MsTimer2::start();
}

void loop()
{
  // Täällä kello käy, eli kun mennään keskeytykseen niin millisekunti-aikaleima jää täältä saatuun arvoon
  // Siksi volatile-tyyppi aika-arvolla... ja tämä mahdollistaa aikaleiman tulostuksen tarvittaessa.

  //Tuotetaan digitaali-output -navalle muuttuva jännite (5V / 0V)
    if (Tila == 0) {
      // turn LED on:
      digitalWrite(OutPin2, HIGH); //Hitaampi pulssitus.
      digitalWrite(OutPin3, HIGH); //Tuplataajuus edelliseen verrattuna.
      viive = OutPulseLength / 2; //Lasketaan tässä viive nopeammalle pulssitukselle.
      delay(viive);
      digitalWrite(OutPin3, LOW);   
      delay(viive);  
      Tila = 1;
    }
    else {
    // turn LED off:
    digitalWrite(OutPin2, LOW);
    digitalWrite(OutPin3, HIGH);
    delay(viive);
    digitalWrite(OutPin3, LOW);   
    delay(viive); 
    Tila = 0;
  }
}
