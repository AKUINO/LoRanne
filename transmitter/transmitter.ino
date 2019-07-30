/*
          _          _                   _____
    /\   | |        (_)                 / ____|
   /  \  | | ___   _ _ _ __   ___ _____| (___   ___ _ __  ___  ___  _ __
  / /\ \ | |/ / | | | | '_ \ / _ \______\___ \ / _ \ '_ \/ __|/ _ \| '__|
 / ____ \|   <| |_| | | | | | (_) |     ____) |  __/ | | \__ \ (_) | |
/_/    \_\_|\_\\__,_|_|_| |_|\___/     |_____/ \___|_| |_|___/\___/|_|

Programme de check-up + envoie des données sur le GateWay via LoRa

*/
#define CLIENT_ADDRESS 4
#define SERVER_ADDRESS 120
uint8_t SENSOR_ID = 4;

#include <Wire.h>
#include <OneWire.h>
#include <SPI.h>
#include <avr/dtostrf.h>
#include "Adafruit_SHT31.h"
#include "pt.h"
#include "FIFO.h"
#include "JBCDIC.h"

Adafruit_SHT31 sht31 = Adafruit_SHT31();

#include <RH_RF95.h>
uint8_t slaveSelectPin = 8, interruptPin = 3;
RH_RF95 radioDriver(slaveSelectPin, interruptPin);
#include <RHReliableDatagram.h>
RHReliableDatagram rhRDatagram(radioDriver, CLIENT_ADDRESS);

/////////////////////////////
//Déclaration des sorties
/////////////////////////////

//Sortie du 1-Wire
int oneWireOutput = 5;
OneWire  ds(oneWireOutput);

///////////////////////////////////////
//Déclaration des variables
//////////////////////////////////////
//Fréquence d'utilisation (par defaut 869.35 MHz)
float frequence_LoRa = 869.35;
//Bande passante utilisée (par defaut : 62,5 KHz)
long bande_passante_LoRa = 62500;
//Puissance utilisée (par defaut : 10 mW)
uint8_t puissance_signal_LoRa = 10;

//Compteur de millisecondes pour vérifier le système de badge
unsigned long previousMillis=0;
//Taille FIXE Maximale du buffer d'envoi LoRa
#define SIZE_BUFFER 100
uint32_t compteur = 0;
FIFO sensorFIFO;
#define ledPin 13
#define v_OneWire 12
#define v_I2C 11
#define v_Analog1 A4
#define v_Analog2 A5
static struct pt sensorAnalysisPT, sendingPT, receivingPT;
uint8_t tmpBuffer[100];

void setup() {
  //Alimentation(Vcc) de la LED
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  //Alimentation(Vcc) du 1-Wire
  pinMode(v_OneWire, OUTPUT);
  digitalWrite(v_OneWire, LOW);
  //Alimentation(Vcc) du I²C
  pinMode(v_I2C, OUTPUT);
  digitalWrite(v_I2C, LOW);
  //Alimentation(Vcc) de l'analogique A4
  pinMode(v_Analog1, OUTPUT);
  digitalWrite(v_Analog1, LOW);
  //Alimentation(Vcc) de l'analogique A5
  pinMode(v_Analog2, OUTPUT);
  digitalWrite(v_Analog2, LOW);
  //On démarre le 1-Wire
  Wire.begin();
  #define VBATPIN A7
  PT_INIT(&sensorAnalysisPT);
  PT_INIT(&sendingPT);
  PT_INIT(&receivingPT);
  if(Serial) {
    //On allume la LED car on est connecté au serial (mode debug)
    digitalWrite(ledPin, HIGH);
    Serial.println("########################################");
    Serial.println("########## Akuino Sensor v17 ##########");
    Serial.println("########################################");
    Serial.print(millis());
    Serial.print(";D;Fréquence:");
    Serial.print(frequence_LoRa);
    Serial.print("MHz");
    Serial.print(";Bande passante=");
    Serial.print(bande_passante_LoRa);
    Serial.print("KHz");
    Serial.print(";Puissance=");
    Serial.print(puissance_signal_LoRa);
    Serial.print("mW");
    Serial.print(";ID=" );
    Serial.println(SENSOR_ID);
  }
}

void loop() {
  Serial.begin(115200);
  Serial1.begin(115200);
  delay(2500);
  //Fréquence d'échantillonnage en min (par defaut : 15min)
  float periode_echantillon = 6; // in seconds

  sensorAnalysis(&sensorAnalysisPT);
  loRaSending(&sendingPT);
  //receiving(&receivingPT);

  if(Serial) {
    Serial.print(millis());
    Serial.println(";D;Sleeping Microchip");
  }
  //On coupe toute les alimentation
  digitalWrite(ledPin, LOW);
  //On attend la fréquence d'échantillonnage avant de relancer
  delay(periode_echantillon*1000); // PROTOTHREAD
  compteur++;
  //Serial.println(compteur);
}

////////////////////////////////////////////////////////////////////////
//Fonction pour la lecture de la température via le chip DS18B20 1-Wire
///////////////////////////////////////////////////////////////////////

float lecture_DS18B20(byte addr[8]) {
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  float celsius, fahrenheit;

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1); // start conversion, with parasite power on at the end

  delay(1000); // maybe 750ms is enough, maybe not  // PROTOTHREAD
  // we might do a ds.depower() here, but the reset will take care of it.

  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE); // Read Scratchpad

  if(Serial) {
    Serial.print("  Data = ");
    Serial.print(present, HEX);
    Serial.print(" ");
  }
  for ( i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
    if (Serial) {
      Serial.print(data[i], HEX);
      Serial.print(" ");
    }
  }
  if(Serial) {
    Serial.print(" CRC=");
    Serial.print(OneWire::crc8(data, 8), HEX);
    Serial.println();
  }

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }

  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  if (Serial) {
    Serial.print("  Temperature = ");
    Serial.print(celsius);
    Serial.print(" Celsius, ");
    Serial.print(fahrenheit);
    Serial.println(" Fahrenheit");
  }
  //on retourne la valeur du capteur
  return(celsius);
}//end function

////////////////////////////////////////////////////////////////////////
//Fonction pour l'analyse des données recues par la GateWay
///////////////////////////////////////////////////////////////////////

void analyse_datas_recues(char* datas_recus) {
  //On extrait les 3 premiers caractères
  char code_message[3];
  memset(code_message, 0, 4);
  strncpy(code_message, datas_recus, 3);
  puts(code_message);
  if(Serial){Serial.println(code_message);}
}

extern int sensorAnalysis(struct pt *pt) {
  PT_BEGIN(pt);
  /////////////////////////////////////////
  //Analyse et lecture des I²C disponibles
  ////////////////////////////////////////
  PT_WAIT_UNTIL(pt, (millis() >= (previousMillis + 30000)) || (millis() < 30000));
  //Variable a envoyer via LoRa
  char buffer_a_envoyer[100]="";
  Serial.println(buffer_a_envoyer);
  Serial.println(strlen(buffer_a_envoyer));
  //pourcentage de la batterie
  float SENSOR_BATTERY;
  byte error, address;
  int nDevices;
  float fonction_return_value;
  char v1_string[5];
  ////////////////////////////////////
  //Récupération du niveau de batterie
  ////////////////////////////////////
  float measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  SENSOR_BATTERY=measuredvbat;
  dtostrf(SENSOR_ID, -1, 0, v1_string);
  snprintf(&buffer_a_envoyer[strlen(buffer_a_envoyer)], SIZE_BUFFER - strlen(buffer_a_envoyer), "M=%s", v1_string);
  //On écrit le header(en-tête) du buffer_a_envoyer + Niveau de batterie
  //le "moins" permet d'écrire à gauche
  dtostrf(SENSOR_BATTERY, -5, 3, v1_string);
  snprintf(&buffer_a_envoyer[strlen(buffer_a_envoyer)], SIZE_BUFFER - strlen(buffer_a_envoyer), "&B=%s", v1_string);

//end if

  ////////////////////////////////////////////////
  //Analyse et lecture des analogiques disponibles
  ////////////////////////////////////////////////
  if(Serial) {
    Serial.println("**********************************************************");
    Serial.println("Analyse et lecture des analogiques disponibles en cours...");
    Serial.println("**********************************************************");
  }
  int pinA0=0;
  int pinA1=0;
  int pinA2=0;

  pinA0=digitalRead(A0);
  pinA1=analogRead(A1);
  pinA2=analogRead(A2);

  if (Serial) {
    Serial.print("PIN_A0 = ");
    Serial.println(pinA0);
    Serial.print("PIN_A1 = ");
    Serial.println(pinA1);
    Serial.print("PIN_A2 = ");
    Serial.println(pinA2);
  }

  //On écrit
  //le "moins" permet d'écrire à gauche
  dtostrf(pinA0, -1, 0, v1_string);
  //On récrit dans la variable du buffer
  snprintf(&buffer_a_envoyer[strlen(buffer_a_envoyer)], SIZE_BUFFER - strlen(buffer_a_envoyer), "&A0=%s", v1_string);
  //On écrit
  //le "moins" permet d'écrire à gauche
  dtostrf(pinA1, -1, 0, v1_string);
  //On récrit dans la variable du buffer
  snprintf(&buffer_a_envoyer[strlen(buffer_a_envoyer)], SIZE_BUFFER - strlen(buffer_a_envoyer), "&A1=%s", v1_string);
  //On écrit
  //le "moins" permet d'écrire à gauche
  dtostrf(pinA2, -1, 0, v1_string);
  //On récrit dans la variable du buffer
  snprintf(&buffer_a_envoyer[strlen(buffer_a_envoyer)], SIZE_BUFFER - strlen(buffer_a_envoyer), "&A2=%s", v1_string);
  if(Serial) {
    Serial.print(millis());
    Serial.println(";I;Begin I²C analysis");
  }

  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    digitalWrite(v_I2C, HIGH);
    /* REMARQUE :
     * Le scanner utilise la valeur de retour de Write.endTransmisstion
     * pour voir si un périphérique a émit un accusé réception de l'adresse
     */
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      if (Serial) {
        Serial.print(millis());
        Serial.print(";I;Sensor found at 0x");
        Serial.println(address,HEX);
      }
      nDevices++;

      //////////////////////////////////
      //SHT31 : Température et humidité
      //Adresse I²C: 0x44
      /////////////////////////////////

      if(address == 0x44) { //Adresse en Hexadécimal
        if (Serial) {Serial.print(millis());}
        if (! sht31.begin(0x44)) {
          if (Serial) {Serial.println(";E;H1 or T1 not connected");}
        }
        else {
          //on lit les informations de ce capteur
          float t = sht31.readTemperature();
          float h = sht31.readHumidity();

          if (!isnan(t)) {  // check if 'is not a number'
            if (Serial) {
              Serial.print(";I;t = ");
              Serial.print(t);
            }
          } else {
            if (Serial) {Serial.println(";W;t = null");}
          }
          if (!isnan(h)) {  // check if 'is not a number'
            if (Serial) {
              Serial.print(";I;H = ");
              Serial.println(h);
            }
          } else {
            if (Serial) {Serial.println(";W;H = null");}
          }
          //le "moins" permet d'écrire à gauche
          dtostrf(t, -5, 3, v1_string);
          //On récrit dans la variable du buffer
          snprintf(&buffer_a_envoyer[strlen(buffer_a_envoyer)], SIZE_BUFFER - strlen(buffer_a_envoyer), "&C1=%s", v1_string);
          //le "moins" permet d'écrire à gauche
          dtostrf(h, -5, 3, v1_string);
          //On récrit dans la variable du buffer
          snprintf(&buffer_a_envoyer[strlen(buffer_a_envoyer)], SIZE_BUFFER - strlen(buffer_a_envoyer), "&H1=%s", v1_string);
        }
      }
    }
    else if (error==4) {
      if(Serial){Serial.print("Erreur inconnue à l'adresse 0x");}
      if (address<16)
      if(Serial) {
        Serial.print(millis());
        Serial.print(";E;Unknown error at 0x");
        Serial.println(address,HEX);
      }
    }
    digitalWrite(v_I2C, LOW);
  }
  if (Serial) {Serial.print(millis());}
  if (nDevices == 0) {
    if(Serial){Serial.println(";E;No device found");}
  }
  else {
    if(Serial){Serial.println(";I;End I²C analysis");}
  }
  ///////////////////////////////////////////////////////
  //Analyse et lecture des 1-Wire disponibles en cours...
  ///////////////////////////////////////////////////////

  if(Serial) {
    Serial.print(millis());
    Serial.println(";I;Begin 1-Wire analysis");
  }
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8];

  if (Serial) {Serial.print(millis());}
  if (!ds.search(addr)) {
    if(Serial){Serial.println(";F;No 1-Wire detected");}
    /*
    if(Serial){Serial.println("Recherche terminée");}

    ds.reset_search();
    delay(2500);

    return;
    */
  }
  else {
  //On récrit dans la variable du buffer
  snprintf(&buffer_a_envoyer[strlen(buffer_a_envoyer)], SIZE_BUFFER - strlen(buffer_a_envoyer), "&G=");
  if (Serial) {
    Serial.print(millis());
    Serial.print(";D;ROM =");
  }
  for( i = 0; i < 8; i++) {
    Serial.write(' ');
    if (addr[i]<16) {
      if (Serial) {Serial.print("0");}
      //le "moins" permet d'écrire à gauche
      dtostrf(0, -0, 0, v1_string);
      //On récrit dans la variable du buffer
      snprintf(&buffer_a_envoyer[strlen(buffer_a_envoyer)], SIZE_BUFFER - strlen(buffer_a_envoyer), "%s", v1_string);
    }
    if(Serial){Serial.print(addr[i], HEX);}
    //le "moins" permet d'écrire à gauche
    dtostrf(addr[i], -0, 0, v1_string);
    //On récrit dans la variable du buffer
    snprintf(&buffer_a_envoyer[strlen(buffer_a_envoyer)], SIZE_BUFFER - strlen(buffer_a_envoyer), "%s", v1_string);
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
    if(Serial){Serial.println(";invalid CRC");}
  }
  if(Serial){Serial.println();}

  if(Serial) {
    Serial.print("CHIP ");
    Serial.print(addr[0],HEX);
    //Le premier octet (ROM) indique quelle est le chip (dipositif)
    Serial.print(" = ");
  }
  switch (addr[0]) {
    case 0x01:
      if(Serial){Serial.println("DS1990 DS2401");}
      break;
    case 0x02:
      if(Serial){Serial.println("DS1991");}
      break;
    case 0x04:
      if(Serial){Serial.println("DS1994");}
      break;
    case 0x05:
      if(Serial){Serial.println("DS2405");}
      break;
    case 0x06:
      if(Serial){Serial.println("DS1992");}
      break;
    case 0x08:
      if(Serial){Serial.println("DS1993");}
      break;
    case 0x0B:
      if(Serial){Serial.println("DS1985");}
      break;
    case 0x10:
      if(Serial){Serial.println("DS1820 DS18S20 DS1920");}
      break;
    case 0x12:
      if(Serial){Serial.println("DS2406");}
      break;
    case 0x21:
      if(Serial){Serial.println("DS1921");}
      break;
    case 0x22:
      if(Serial){Serial.println("DS1822");}
      break;
    case 0x24:
      if(Serial){Serial.println("DS1904");}
      break;
    case 0x28:
      if(Serial){Serial.println("DS18B20");}
      if(Serial){Serial.println("Lecture des données de ce chip...");}
      //On appelle la fonction pour la lecture
      fonction_return_value = lecture_DS18B20(addr);
      //le "moins" permet d'écrire à gauche
      dtostrf(fonction_return_value, -5, 3, v1_string);
      //On récrit dans la variable du buffer
      snprintf(&buffer_a_envoyer[strlen(buffer_a_envoyer)], SIZE_BUFFER - strlen(buffer_a_envoyer), "/DS18B20=%s", v1_string);
      break;
    case 0x29:
      if(Serial){Serial.println("DS2408");}
      break;
    case 0x36:
      if(Serial){Serial.println("DS2740");}
      break;
    case 0x3B:
      if(Serial){Serial.println("DS1825");}
      break;
    case 0x41:
      if(Serial){Serial.println("DS1923");}
      break;
    default:
      if(Serial){Serial.println(" n'est pas répertorié.");}
      break;
    }
  }
  if(Serial) {
    Serial.print(millis());
    Serial.println(";I;End 1-Wire analysis");
  }
  previousMillis = millis();
  if(Serial) {
    Serial.println("----------------------------------------------- FIN DES SCANS -----------------------------------------------");
    Serial.print(millis());
    Serial.print(";I;buffer = ");
    Serial.println(buffer_a_envoyer);
    Serial.println(strlen(buffer_a_envoyer));
  }
  sensorFIFO.pushBuffer((uint8_t*)buffer_a_envoyer, strlen(buffer_a_envoyer));
  //Si on est en mode DEBUG, alors on laisse un petit delai
  PT_END(pt);
}

static int loRaSending(struct pt *pt) {
  PT_BEGIN(pt);
  //////////////////////////////////
  //ENVOIE DES INFORMATIONS VIA LoRa
  //////////////////////////////////
  PT_WAIT_UNTIL(pt, !(sensorFIFO.isEmpty()));
  if(Serial) {
    Serial.println("*****************************************************");
    Serial.println("Transmission des données via LoRa (RF95)");
    Serial.println("*****************************************************");
  }
  
  if (Serial) {Serial.print(millis());}
  //Activation du LoRa
  if (!rhRDatagram.init()) {
    if(Serial){Serial.println(";E;LoRa not initialized");}
  } else {
    if(Serial){Serial.println(";I;LoRa initialized");}
  }
  if (Serial) {
    Serial.print(millis());
    Serial.print(";I;FIFO size = ");
    Serial.println(sensorFIFO.size());
  }
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  radioDriver.setFrequency(frequence_LoRa);
  radioDriver.setTxPower(puissance_signal_LoRa);
  radioDriver.setSignalBandwidth(bande_passante_LoRa);
  if (Serial) {Serial.print(millis());}
  if (!(sensorFIFO.isEmpty())) {
    sensorFIFO.peekBuffer(tmpBuffer, SIZE_BUFFER);
    JBCDIC jbcdic;
    uint8_t tmpBuffer2[SIZE_BUFFER];
    int counter = jbcdic.encode_to_jbcdic((char*)tmpBuffer, sensorFIFO.peek(), tmpBuffer2, SIZE_BUFFER);
    Serial.println("*******************");
    Serial.println(counter);
    for(int i = 0; i < strlen((char*)tmpBuffer2); i++) {
      Serial.print((char)tmpBuffer2[i]);
    }
    Serial.println("");
    Serial.println("*******************");
    
    if(rhRDatagram.sendtoWait(tmpBuffer2, sensorFIFO.peek(), SERVER_ADDRESS)) {
      sensorFIFO.popBuffer(tmpBuffer, SIZE_BUFFER);
      if (Serial) {Serial.println(";I;Packet sent");}
      // Now wait for a reply from the server
      uint8_t len = sizeof(buf);
      uint8_t from;
      if (rhRDatagram.recvfromAckTimeout(buf, &len, 2000, &from)) {
        buf[len] = '\0';
        Serial.print("got reply from : 0x");
        Serial.print(from, HEX);
        Serial.print(": ");
        Serial.println((char*)buf);
      }
      else {
        Serial.println("No reply, is rf95_reliable_datagram_server running?");
      }
    }
    else{
      if (Serial) {
        Serial.println(";W;Packet not sent");
        Serial.println(sensorFIFO.size());
      }
    }
  }
  PT_END(pt);
}

/*extern int receiving(struct pt *pt) {
  PT_BEGIN(pt);
  PT_WAIT_UNTIL(pt, sensorFIFO.size() < tmpQueueSize);
  //PT_WAIT_UNTIL(pt, sensorFIFO.size() < tmpQueueSize);
  //Délai d'attente d'une réponse LoRa (par defaut : 5 secondes)
  int delai_reponse_LoRa = 1;
  long duree_attente_aleatoire;
  tmpQueueSize--;
  //On attend que le packet ait bien été envoyé...
  // Maintenant, on attend une éventuelle réponse de notre GateWay
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  uint8_t from;
  uint16_t timeout = 5000;
  char verification[4];
  char tmp[1];
  dtostrf(SENSOR_ID, -1, 0, tmp);
  snprintf(verification, 4, "%s%s", "M=", tmp);

  if (rhRDatagram.waitAvailableTimeout(delai_reponse_LoRa*1000)) {
    // Should be a reply message for us now
    if (rhRDatagram.recvfromAckTimeout(buf, &len, timeout, &from)) {
      if(Serial) {
        Serial.print(millis());
        Serial.print(";D;Response Received");
        Serial.print(";RSSI: ");
        Serial.print(radioDriver.lastRssi(), DEC);
        RH_RF95::printBuffer(";Received datas [HEXA]: ", buf, len);
      }
      //On vide le buffer_a_envoyer car il a bien été recu...
      //memset(buffer_a_envoyer, 0, sizeof(buffer_a_envoyer));

      //On traite la réponse reçue via notre fonction de traitement de données recues
      //analyse_datas_recues((char*)buf);
      //On verifie si le recepteur a bien recu notre réponse...
      if (String((char*)buf) == verification) {
        if (Serial) {
          Serial.print(millis());
          Serial.println(";D;GATEWAY got response");
        }
        delay(5000); // PROTOTHREAD
      }
      else {
        if (Serial) {
          Serial.print(millis());
          Serial.println(";W;Interference");
        }
        //on attend une durée aléatoire avant de renvoyé!
        duree_attente_aleatoire = random(1000,15000);
        if (Serial) {
          Serial.print("on attend ");
          Serial.print(duree_attente_aleatoire);
          Serial.println("ms avant de re envoyé...");
        }
        delay(duree_attente_aleatoire); // PROTOTHREAD
        if (Serial) {
          Serial.println(verification);
          Serial.println("On envoie...");
        }
        //char* buffer_a_envoyer = removeFromFIFO(sensorFIFO);
        sensorFIFO.peekBuffer(tmpBuffer, 100);
        for(int i = 0; i < sensorFIFO.peek(); i++) {
          Serial.print((char)tmpBuffer[i]);
        }
        if(rhRDatagram.sendtoWait(tmpBuffer, sensorFIFO.peek(), SERVER_ADDRESS)) { // PROTOTHREAD
          if(Serial){
            Serial.print(millis());
            Serial.println(";D;Packet sent");
          }
          for(int i = 0; i < sensorFIFO.peek(); i++) {sensorFIFO.pop();}
        }
        else {
          Serial.println(";W;Packet not sent");
        }
      }
    }
    else {
      if (Serial) {
        Serial.print(millis());
        Serial.println(";W;Listening Problem");
      }
    }
  }
  else {
    if (Serial) {
      Serial.print(millis());
      Serial.println(";W;No response");
    }
  }
  PT_END(pt);
}*/
