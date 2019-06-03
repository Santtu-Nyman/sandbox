/*

Tämän ohjelman on tarkoitus lukea näppäimistöltä kokonaisluku väliltä 0 - 255 (8 bitillä tiloja on 2^8 = 256 eli nollasta alkaen suurin luku on 255)
tai sitten voit käyttää jotain muutakin bittimäärää ja testata lukuja sen mukaisesti.

Tällä ohjelmalla siis demotaan analogisen ja digitaalisen esitystavan piirteitä.

Tämä ohjelma vaatii toimiakseen että avaat SerialMonitorin ja syötät sillä luvun jonka haluat binäärimuotoon muuntaa.
Tämä ohjelma näyttää bittijonon sarjamonitorilla niin, että ensimmäinen ykkönen toimii aloitussignaalina ja tämän jälkeen tulee binäärisenä annettu luku.

Tämä ohjelma tulostaa bittijonon myös sähköisenä OutPin-navasta, jota voidaan toisella Arduinolla tai vaikka oskilloskoopilla mitata.

TEHTÄVÄ: Syötä tällä ohjelmalla sarjamonitorille eri lukuja. Mittaa toisella Arduinolla tai oskilloskoopilla OutPin-navan sähköistä signaalia ja selvitä sen perusteella syötetty luku.
Testaa aluksi syöttämällä luku 1 että näet kummassa reunassa on vähiten merkitsevä bitti (LSB) ja samalla voit laskea moniko-bittinen toteutus on mitattavana.

Arduino siis muuntaa annetun luvun binääriluvuksi ja työntää sen mukaisesti bittijonoa jota voidaan katsoa SerialPlotter-toiminnolla tai OutPin-navasta toisella laitteella.

Aloitusmerkkinä on n. 1 sekunnin HIGH-tila. Tämän jälkeen tulee n. sekunnin ajan kutakin bittiä niin että bittien ajastusta on korostettu nopeasti värähtävällä bitinaloitusmerkillä.

*/

const int OutPin = 3;           // Output-pinni on ilman A-etuliitettä (A3 on analogia-pinni), eli nyt käytössä on digitaalipuolella numero 3, joka on PWM-navaksi myös merkitty.
const int bitteja = 8;         // Voit säätää bittimäärääkin jos haluat.
int readValue = 0;              // Näppäimistöltä luettava luku, maksimissaan 255 (8 bittiselle systeemille)
int bitValue[bitteja] = {0};

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);
  // make the pins outputs:
  pinMode(OutPin, OUTPUT);
}

void loop() {

  // if there's any serial available, read it:
  if (Serial.available() > 0) {
    // Jos sarjaportille on kirjoitettu luku niin alkaa tämä osio jossa kijoitetaan ensin aloitusmerkki ja sitten tulostetaan luku binäärisenä:
    Serial.println(1);
    analogWrite(OutPin, 255);     //Nitkutetaan aluksi edestakaisin lyhyellä viiveellä jotta saadaan bitin ajastus havainnollisemmin näkyville.
    delay(20);
    analogWrite(OutPin, 0);
    delay(20);
    analogWrite(OutPin, 255);    //Ja sitten kirjoitetaan pidemmän aikaa sitä varsinaista bittiä.
    delay(1000);
    
    // look for the next valid integer in the incoming serial stream:
    int readValue = Serial.parseInt();

    // Kun sarjaportille syötetään luku, alkaa tulostus. Pitää aloittaa bitistä MSB, eli tehdään ohjelmallinen AD-muunnos:
      for (int lkm = bitteja - 1; lkm > -1; lkm--)
      {
        if(readValue > (pow(2,lkm) - 0.9))        // Tässä 0.9:n sijaan loogisempi olisi ollut luku 1, mutta se ei yhtäsuuruustilanteessa toiminut odotetulla tavalla.
        {
          bitValue[lkm] = 1;                      // Jos syötetty arvo on yli testatun lukuarvon niin kyseinen bitti asetetaan ykköseksi
          readValue = readValue - (pow(2,lkm));   // ja miinustetaan sitä vastaava luku pois jotta tämän päällä olevaa lukua voidaan jatkossa muuntaa pienemmillä biteillä.
        }
        else                                      // Ja jos ei oltu yli testatun bittisuuruuden niin bitti jää nollaksi.
        {
          bitValue[lkm] = 0;
        }
/*  
        Serial.print("eka");
        Serial.print("\t");
        Serial.print(lkm);
        Serial.print("\t");
        Serial.print(pow(2,lkm));
        Serial.print("\t");
        Serial.println(bitValue[lkm]);
*/
      }

      
      // Tulostus sarjaportille ja output-navalle: Analogwrite kirjoittaa OutPin-navalle sähköisen viestin ja Serial.print kijoittaa sarjaportille
      
      for (int lkm = bitteja - 1; lkm > -0.5; lkm--)
      {
        analogWrite(OutPin, bitValue[lkm]*255);
        delay(30);
        analogWrite(OutPin, 0);
        delay(30);
        analogWrite(OutPin, 255); 
        delay(30);
        analogWrite(OutPin, 0);
        delay(30);
        analogWrite(OutPin, bitValue[lkm]*255);
        //Serial.print(lkm);
        //Serial.print("\t");
        Serial.println(bitValue[lkm]);
        delay(500);
        // Ja syrppyä loppuun:
        analogWrite(OutPin, 0);
        delay(30);
        analogWrite(OutPin, 255); 
        delay(30);
        analogWrite(OutPin, 0);
      }
      //Serial.println("tauko");
  }
  // if päättyi joten tulostetaan nollaa ilman mitään bitti-aloitusmerkkejä. Tavallaan "lähetys ei ole päällä" ja mitään sähköistä viestiä ei ole liikkeellä.
      Serial.println(0);
      analogWrite(OutPin, 0);
      delay(1000);
 }
