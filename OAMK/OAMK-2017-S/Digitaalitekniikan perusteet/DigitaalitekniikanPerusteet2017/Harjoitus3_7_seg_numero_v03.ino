/*

Digitaalitekniikka / Labraharjoitus 3
7-segmentti -näyttöpalikan, eli yhden numeron kirkkauden ohjaus pelkästään Arduinolla.

Tarkoitus on syöttää SerialMonitorilla luku 0 - 255 PWM-ulostulolle ja testata ledin kirkkautta sen funktiona.

Pohjalla oli edellinen harjoitusversio, mutta tässä sieltä otettuja ohjeita:

1. Selvitä ensiksi näyttöpalikan kytkentälogiikka. Etsi webistä pin-konfiguraatio ja katso siitä GND-kytkentänapa.

2. Jos liität GND-johtimeen 500 ohmin vastuksen niin se toimii virran rajoittimena samalla kertaa kaikille ledeille.
Tällöin kuitenkin ledien kirkkaus muuttuu riippuen palavien ledien lukumäärästä.
Jos tämä ei haittaa, niin selvitään pienemmällä komponenttimäärällä. Parhaan ja tasaisen kirkkauden saat 7-näytölle jos kytket kullekin 
ledille oman sarjavastuksensa.

3. Käytä kytkennässäsi seuraavaa numerointia. Keskellä oleva viiva olkoon PIN NRO 8. PISTE kytketään suoraan 5V-napaan tai 9-napaan,
koska se saa olla koko ajan päällä.
Ohjelmalogiikan mukainen Arduinon digitaalipuolen pinnien numerointi suhteessa 7-segmenttinäytön ledien sijaintiin:

    6
    _
 7 | | 5
    -
 4 | | 2
    -
    3
 

HUOM! Tässä ohjelmaversiossa tulostetaan jatkuvasti lukuarvoa 3. Kaikki sen LEDit eivät ole PWM-napoja, 
mutta silti tämä ohjelma yrittää kirjoittaa sarjaportilla syötettyä PWM-arvoa jännitteenä ulos. Maksimiarvo = 255 ja minimi = 0.

 */


// Määritellään Output-pinnit:
const int OutPin2 = 2;
const int OutPin3 = 3;
const int OutPin4 = 4;
const int OutPin5 = 5;
const int OutPin6 = 6;
const int OutPin7 = 7;
const int OutPin8 = 8;
const int OutPin9 = 9;

int readValue = 0;    // Sarjamonitorin avulla syötettävä jännitettä kuvaava kokonaisluku välillä 0 - 255.
boolean sisaan = true;


void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);
  // make the pins outputs:
  pinMode(OutPin2, OUTPUT);
  pinMode(OutPin3, OUTPUT);
  pinMode(OutPin4, OUTPUT);
  pinMode(OutPin5, OUTPUT);
  pinMode(OutPin6, OUTPUT);
  pinMode(OutPin7, OUTPUT);
  pinMode(OutPin8, OUTPUT);
  pinMode(OutPin9, OUTPUT);
}

void loop() {

  //Vain alkuun tulostetaan tekstit, joka vaatii Genuinolla sarjamonitori-yhteyden tarkkailua
  while (!Serial) sisaan = true;  //  wait for the serial monitor to open
  if (sisaan) {
    Serial.println("Anna jokin kokonaisluku valilla 0 - 255 ja paina Enter");
    Serial.println("Tämä ohjelma tulostaa numeroa 3 niin että osa segmenteista");
    Serial.println("voidaan saataa himmeammalle tai kirkkaammalle PWM-syotolla");
    Serial.println("mittaa PWM-signaali myos skoopilla ja vaihtele syottoarvoa... mita tapahtuu?");
    sisaan = false;
  }

  
  // if there's any serial available, read it:  
  if (Serial.available() > 0) {
    
    // look for the next valid integer in the incoming serial stream:
    readValue = Serial.parseInt();              //Luetaan käyttäjän syöttämä luku
    
    //readValue = constrain(readValue,0,255);         //Pakotetaan syötetty arvo vaaditulle välille.

    // tulostetaan luku 3 mutta sarjaportilla määrätyllä voimakkuudella PWM-navoille:
      digitalWrite(OutPin2, HIGH);
      analogWrite(OutPin3, readValue);
      digitalWrite(OutPin4, LOW);
      analogWrite(OutPin5, readValue);
      analogWrite(OutPin6, readValue);
      digitalWrite(OutPin7, LOW);
      digitalWrite(OutPin8, HIGH);
      analogWrite(OutPin9, readValue);
      delay(2);

    //Tulostetaan syöttötietoja ruutuun:
    Serial.print("Syotit PWM-luvun ");
    Serial.println(readValue);  
    delay(1000);

  }
    delay(2);                                      //Rauhoitetaan silmukkaa hieman
}
