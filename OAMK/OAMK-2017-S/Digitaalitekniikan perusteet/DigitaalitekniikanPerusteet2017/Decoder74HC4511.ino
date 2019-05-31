/* 
 
Koodataan dekooderi 74HC4511, 
joka lukee numeron sarjaportilta 
ja lähettää sen dekooderille 7-segmentti-tulostusta varten

 */



// Piirille 74HC4511 menevät BCD-bittisyötöt
// http://www.nxp.com/documents/data_sheet/74HC_HCT4511_CNV.pdf
const int D1 =  3; 
const int D2 =  4; 
const int D3 =  5;
const int D4 =  6;

// Katso datalehdeltä, mitä pitää kytkeä 
// navoille LE, BI ja LT, jotta dekooderi päivittää luvun eteenpäin 
// 7-segmenttinäytölle. 

int readValue = 0;    // Sarjamonitorin avulla syötettävä jännitettä kuvaava kokonaisluku välillä 0 - 9.
boolean sisaan = true;

void setup() {
  // Asetetaan napojen roolit:
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D4, OUTPUT);
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);
}

void loop() {
  // Tämä loop-silmukka on se ajettava ohjelma, joka pyörii ympäri niin kauan kuin Arduinolla on virtaa.

  //Vain alkuun tulostetaan tekstit, joka vaatii Genuinolla sarjamonitori-yhteyden tarkkailua
  while (!Serial) sisaan = true;  //  wait for the serial monitor to open
  if (sisaan) {
    Serial.println("Anna jokin kokonaisluku valilla 0 - 9 ja paina Enter");
    Serial.println("Jos syotat kirjaimen, saat nollan");
    Serial.println("Jos syotat muun luvun, se pakotetaan valille 0 - 9");
    sisaan = false;
  }

    // if there's any serial available, read it:  
  if (Serial.available() > 0) {
    
    // look for the next valid integer in the incoming serial stream:
    readValue = Serial.parseInt();              //Luetaan käyttäjän syöttämä luku
    
    //readValue = constrain(readValue,0,9);         //Pakotetaan syötetty arvo vaaditulle välille.

      //Tulostetaan tietoa sarjaportille:
      Serial.print(" Annoit luvun ");
      Serial.print(readValue);
      Serial.println(", saatko sen 7-segmenttinaytolle?");
      delay(10);
  }

//Jos syötettiin nolla:
    if(readValue == 0)                             
    {
      digitalWrite(D1, LOW);
      digitalWrite(D2, LOW);
      digitalWrite(D3, LOW);
      digitalWrite(D4, LOW);
      delay(2);
    }

//Jos syötettiin yksi:
    if(readValue == 1)                             
    {
      digitalWrite(D1, HIGH);
      digitalWrite(D2, LOW);
      digitalWrite(D3, LOW);
      digitalWrite(D4, LOW);
      delay(2);
    }

//Jos syötettiin kaksi:
    if(readValue == 2)                             
    {
      digitalWrite(D1, LOW);
      digitalWrite(D2, HIGH);
      digitalWrite(D3, LOW);
      digitalWrite(D4, LOW);
      delay(2);
    }

//Jos syötettiin kolme:
    if(readValue == 3)                             
    {
      digitalWrite(D1, HIGH);
      digitalWrite(D2, HIGH);
      digitalWrite(D3, LOW);
      digitalWrite(D4, LOW);
      delay(2);
    }

//Jos syötettiin neljä:
    if(readValue == 4)                             
    {
      digitalWrite(D1, LOW);
      digitalWrite(D2, LOW);
      digitalWrite(D3, HIGH);
      digitalWrite(D4, LOW);
      delay(2);
    }

//Jos syötettiin viisi:
    if(readValue == 5)                             
    {
      digitalWrite(D1, HIGH);
      digitalWrite(D2, LOW);
      digitalWrite(D3, HIGH);
      digitalWrite(D4, LOW);
      delay(2);
    }

//Jos syötettiin kuusi:
    if(readValue == 6)                             
    {
      digitalWrite(D1, LOW);
      digitalWrite(D2, HIGH);
      digitalWrite(D3, HIGH);
      digitalWrite(D4, LOW);
      delay(2);
    }

//Jos syötettiin seitsemäm:
    if(readValue == 7)                             
    {
      digitalWrite(D1, HIGH);
      digitalWrite(D2, HIGH);
      digitalWrite(D3, HIGH);
      digitalWrite(D4, LOW);
      delay(2);
    }

//Jos syötettiin kahdeksan:
    if(readValue == 8)                             
    {
      digitalWrite(D1, LOW);
      digitalWrite(D2, LOW);
      digitalWrite(D3, LOW);
      digitalWrite(D4, HIGH);
      delay(2);
    }

//Jos syötettiin yhdeksän:
    if(readValue == 9)                             
    {
      digitalWrite(D1, HIGH);
      digitalWrite(D2, LOW);
      digitalWrite(D3, LOW);
      digitalWrite(D4, HIGH);
      delay(2);
    }
}

