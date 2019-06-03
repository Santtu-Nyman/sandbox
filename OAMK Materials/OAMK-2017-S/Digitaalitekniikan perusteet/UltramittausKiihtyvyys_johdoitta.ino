/*
 * 
 HC-SR04 ultraäänianturilla voidaan mitata etäisyyttä ajan funktiona.
 Näytteistystaajuutta rajoittaa äänen kulkunopeus ilmassa, noin 340m/s
 Tästä voit laskea aikaviiveen, kun ääni lähtee anturista kohteeseen ja palaa takaisin.
 Anturitiedot löydät kivasti Googlettamalla anturin tyyppimerkinnällä.
 Tässä ohelmassa kytkentänavat on ohjelmoitu niin että erillisiä hyppylankoja ei tarvita, vaan
 anturi voidaan kytkeä suoraan Arduino UNO-alustalle napoihin 8 - 11 siten, että "silmät" osoittavat
 boardista poispäin.
 
 1.10.2016 -Jaakko Kaski-
 
 */

// Määritellään kytkentänavat ultraäänitutkalle HC-SR04
 const int GNDPin = 11; // ultraäänianturin maa-napa
 const int echoPin = 10; // Echo Pin (kaiku, eli vastaanotinpuoli)
 const int trigPin = 9; // Trigger Pin (ultraääni-lähtetin)
 const int VccPin = 8; // Anturin käyttöjännite

// Määritellään kytkentänavat kiihtyvyysanturille:
 const int VccPin2 = A0;  // Käyttöjännite
 const int xPin   = A1;   // x-kanavan mittaus
 const int yPin   = A2;   // y-kanava
 const int zPin   = A3;   // z-kanava
 const int GNDPin2 = A4;  // laitteen maa-napa

// Muuttujamäärittelyt. Huomaa, että desimaalierotin on piste!
int maximumRange = 300.0; // Maksimietäisyys (cm) jota tällä on tarkoitus mitata; pidempääkin voit kokeilla
int minimumRange = 2.0; // Minimietäisyys (cm). Lyhyellä etäisyydellä sivusuuntainen kulkumatka tulee merkittäväksi
unsigned long duration = 0; // Lähetetyn uä-pulssin kulkuaika mikrosekunteina
float distance = 0.0; // Äänen kulkuaika kohteeseen ja takaisin; etäisyys (cm)
unsigned long aika = 0; // Aikaleima (ms), tyyppinä "pitkä, merkitön" muoto, koska INT-tyyppisenä numeroavaruus tulee n. puolessa minuutissa täyteen.
int SisaanTunniste = 0; // Käyttöjännitteen asettamiseen liittyvä tunniste. Reagoidaan kun mennään ohjelmaan
                        // eka kerran sisään.
 int x = 0; //x-kanavan sensoriarvo
 int y = 0;
 int z = 0;
 float ax = 0.0;  // x-kanavan kiihtyvyysarvo SI-muodossa (m/s^2)
 float ay = 0.0;
 float az = 0.0;

// Alustetaan kytkentänavat ja sarjayhteys
void setup() {
 Serial.begin (19200); // Tämä täytyy valita myös Serial Monitorista samaksi
 
 // Ultraäänianturin napojen määrittely:
 pinMode(GNDPin, OUTPUT); // Maadoitus; tämäkin on output-napa joka antaa 0V:n jännitteen
 pinMode(echoPin, INPUT);
 pinMode(trigPin, OUTPUT);
 pinMode(VccPin, OUTPUT); // Käyttöjännite
 
 // Kiihtvyys-anturi:
 pinMode(VccPin2, OUTPUT);     // Kiihtyvyysanturin käyttöjännite Vcc
 pinMode(GNDPin2, OUTPUT);     // Kiihtyvyysanturin GND

// Asetetaan syöttöjännite (5V UNO-BOARDILLA, 3.3V Genuino 101:llä) ja maa-arvot (0V):
    digitalWrite(VccPin, HIGH);
    delayMicroseconds(2); 
    digitalWrite(VccPin2, HIGH);
    delayMicroseconds(2); 
    digitalWrite(GNDPin, LOW); 
    delayMicroseconds(2); 
    digitalWrite(GNDPin2, LOW); 
    delayMicroseconds(2);

    while(Serial.available() != 0)
     {
        // Odotellaan että yhteys käynnistyy jos tässä sattuu olemaan viivettä. 0 tarkoittaa että yhteys on.
     } 

}

void loop() {

  // eka sisäänmenolla annetaan 1ms aikaa käynnistyä. Muuten 1. arvo on pelkkää häiriötä.
  if(SisaanTunniste == 0)
  {
    delay(1); // 1ms viive käynnistymiselle
    SisaanTunniste = 1; // muutetaan testattava muuttuja jotta tänne ei enää tulla
  }
     
// trigPin/echoPin kierros määrittää etäisyyden kohteeseen, josta lähtenyt äänipulssi heijastuu takaisin
 digitalWrite(trigPin, LOW); 
 delayMicroseconds(2); 
 digitalWrite(trigPin, HIGH);
 delayMicroseconds(10); 
 digitalWrite(trigPin, LOW);

 duration = pulseIn(echoPin, HIGH); // Äänen kulkuaika mikrosekunteina saadaan kaiusta

 // Aikaleima (ms)
 aika = millis(); // Aikaleima luetaan heti kun etäisyys on mitattu ja juuri mennään lukemaan kiihtyvyydet

 // Kiihtyvyysmittaus
 x = analogRead(xPin);  // Luetaan x-sensoriarvo, joka tulee 10-bittisellä koodauksella eli lukualueella 0 - 1023. 0 = 0V ja 1023 = Vcc.
 y = analogRead(yPin);
 z = analogRead(zPin);

 // Huomaa että kaikki mittaukset tehtiin nipussa, jotta aikaleima olisi niille mahdollisimman sama.
 // Sen jälkeen vasta tehdään laskentaa...
 // Kalibroinnit kiihtyvyydelle lasketaan ennen tulostusta. 
 // Muunnos sensoriarvosta SI-muotoon. Selvitetty kalibrointimittauksella.
 // Huom! tarkka kalibrointi edellyttää että teet itse mittauksen ja selvität kullekin kanavalle tarkan yhtälön.
 // Tämä kalibrointi on vain "tyypillinen" tilanne, joka antaa likimäärin oikeat tulokset.
 // Voit testata kalibroinnin maan vetovoiman avulla. Akseli ylös = 9,81m/s^2 ja alas sama negatiivisena
 // kun et muuten liikuttele anturia.
 ax = 0.1411 * x - 47.218; //Kalibrointiyhtälö x-akselin sensoriarvosta x-suunnan kiihtyvyydeksi.
 ay = 0.1411 * y - 47.218;
 az = 0.1411 * z - 47.218;
 
 //Lasketaan etäisyys (cm) perustuen äänen nopeuteen ilmassa. 
 //Voit selvittää fysiikasta mistä jakaja tulee... "etäisyysLaskelma.xlsx"
 distance = duration/58.3;
 
 if (distance >= maximumRange || distance <= minimumRange){
 // Jos etäisyystulos on epäkelpo, lähetetään negatiivinen tulos periaatteella: "out of range" eli -50(cm)
 Serial.print(aika);
 Serial.print(" ");
 Serial.print("-50");
 Serial.print(" ");
 Serial.print(ax);
 Serial.print(" ");
 Serial.print(ay);
 Serial.print(" ");
 Serial.println(az); 
 }
 else {
 // Tulostetaan tulokset sarjaportille kun mittaus on onnistunut myös etäisyyden osalta.
 // Kiihtyvyys-arvot ovat yleensä kunnossa jos kytkennät on kunnossa.
 Serial.print(aika);          // Aikaleima(ms)
 Serial.print(" ");           // Välilyönti... tämä on tunniste Exceliin siirrossa!
 Serial.print(distance);      // Etäisyys (cm)
 Serial.print(" ");           
 Serial.print(ax);            // x-suunnan kiihtyvyys (m/s^2)
 Serial.print(" ");          
 Serial.print(ay);            // y-suunnan kiihtyvyys (m/s^2)
 Serial.print(" ");
 Serial.println(az);          // z-suunnan kiihtyvyys (m/s^2)
 }
 
 //Viive ennen seuraavaa kierrosta niin dataa tulee kohtuullisella tahdilla
 // Jos mittaa hirmuisen nopeasti niin numerinen derivaatta näyttää kohinaisemmalta 
 // vaikka siinä toki olisi tietoa enemmän!
 delay(2);
}
