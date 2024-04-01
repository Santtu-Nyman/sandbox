/*

Havainnollistetaan Arduinon mittaavaan A0-napaan liitetyn hyppylangan toimintaa antennina.
Samalla toki havainnollistetaan Arduinolla tehtävää mittausta jänniteväliltä 0 - 5000mV.

1.  Valitse Tools / Board sen mukaan mitä Arduino-alustaa käytät.
2.  Valitse Tools / Port. Portin tiputusvalikossa näkyy missä portissa Arduino on, mutta numeron näkee myös laitehallinnan kautta jos tarvii.
3.  Lataa tämä ohjelma Arduino-alustalle.
4.  Tökkää hyppylanka A0-napaan ja jätä toinen pää ilmaan. 
5.  Avaa Tools / Serial Plotter. Kokeile liikkua laitteen lähellä ja huomaat aiheuttavasi sähköisiä "häiriöitä" antennille.
6.  Liitä vapaana oleva pää Arduino-boardin GND-napaan, sitten 3,3V-napaan ja sitten 5V-napaan. 
    Huomaat, että voit käyttää Arduinoa mittaamaan jännitesignaalia välillä 0 - 5V ajan funktiona.
    Huomaat myös, että muuttujien tyyppimäärittely vaikuttaa digitaalisten laitteiden tarkkuuteen.
-Jaakko Kaski-

*/

const int analogInPin = A0;   // Nimetään mittaava Arduinon kytkentäpinni
int sensorValue = 0;          // Muuttujanimi, jolle luetaan sensoriarvo. 0 = 0V ja 1023 on maksimi, joka vastaa 5V. Sensoriarvo on kokonaisluku joka esitetään 10 bitillä: 2^10 = 1024.
float voltage = 0.0;          // Sensoriarvo muutettuna volteiksi. Desimaaliluku jolla on kuitenkin bittitilojensa mukainen tarkkuus (askelväli): 5V / (2^10 - 1) = 4,888mV.
int voltage2 = 0;             // Sensoriarvo muutettuna volteiksi. Kokonaisluku.
boolean sisaan = true;

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);
  //while (!Serial);  //  wait for the serial monitor to open
}

void loop() {
  while (!Serial) sisaan = true;  //  wait for the serial monitor to open
  if (sisaan) {
    Serial.println("Nyt mennaan sisaan ohjelmaan");
    sisaan = false;
  }

  // read the analog in value:
  sensorValue = analogRead(analogInPin);

  // muunnos volteiksi:
  voltage = (float) sensorValue * 5.0 / 1023.0;   // Huomaa että näissä kahdessa on erilaiset muuttujatyypit, muuten samat
  voltage2 = sensorValue * 5 / 1023;              // Huomaa että näissä kahdessa on erilaiset muuttujatyypit, muuten samat

  // Tulostus sarjaportille
  Serial.print(voltage);
  Serial.print("\t");                             // Arduinon SerialPlotter ottaa kahden lukusarakkeen väliin joko välilyönnin tai tabulaattorin \t.
  Serial.println(voltage2);

  // wait some milliseconds before the next loop
  delay(20);
}
