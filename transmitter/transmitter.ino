/*
          _          _                   _____
    /\   | |        (_)                 / ____|
   /  \  | | ___   _ _ _ __   ___ _____| (___   ___ _ __  ___  ___  _ __
  / /\ \ | |/ / | | | | '_ \ / _ \______\___ \ / _ \ '_ \/ __|/ _ \| '__|
 / ____ \|   <| |_| | | | | | (_) |     ____) |  __/ | | \__ \ (_) | |
/_/    \_\_|\_\\__,_|_|_| |_|\___/     |_____/ \___|_| |_|___/\___/|_|
Programme de check-up + envoie des données sur le GateWay via LoRa
*/
// Could be an interactive compilation parameter
#define SENSOR_ID 4
#define SERVER_ADDRESS 120
#define SENSOR_INTERVAL 29
#define EPOCH_TIME 946684800

#include <Wire.h>
#include <OneWire.h>
#include <SPI.h>
#include <avr/dtostrf.h>
#include "Adafruit_SHT31.h"
#include "FIFO.h"
#include "pt.h"
#include "JBCDIC.h"

#include <RTCZero.h>
RTCZero zerortc;

Adafruit_SHT31 sht31 = Adafruit_SHT31();
#include <RH_RF95.h>

// From Arduino library Lora.h
#ifdef ARDUINO_SAMD_MKRWAN1300
#define LORA_DEFAULT_SPI           SPI1
#define LORA_DEFAULT_SPI_FREQUENCY 250000
#define LORA_DEFAULT_SS_PIN        LORA_IRQ_DUMB
#define LORA_DEFAULT_RESET_PIN     -1
#define LORA_DEFAULT_DIO0_PIN      -1
#elif ADAFRUIT_FEATHER_M0
#define LORA_DEFAULT_SPI           SPI
#define LORA_DEFAULT_SPI_FREQUENCY 8E6 
#define LORA_DEFAULT_SS_PIN        8
#define LORA_DEFAULT_RESET_PIN     -1
#define LORA_DEFAULT_DIO0_PIN      3
#else
#define LORA_DEFAULT_SPI           SPI
#define LORA_DEFAULT_SPI_FREQUENCY 8E6 
#define LORA_DEFAULT_SS_PIN        10
#define LORA_DEFAULT_RESET_PIN     9
#define LORA_DEFAULT_DIO0_PIN      2
#endif

RH_RF95 radioDriver(LORA_DEFAULT_SS_PIN, LORA_DEFAULT_DIO0_PIN);
#include <RHReliableDatagram.h>
RHReliableDatagram rhRDatagram(radioDriver, SENSOR_ID);

/////////////////////////////
//Déclaration des sorties
/////////////////////////////
#ifdef ARDUINO_SAMD_MKRWAN1300
  //Sortie du 1-Wire
  #define ONEWIRE_OUTPUT ???
  #define ledPin ???
  #define v_OneWire ???
  #define v_I2C ???
  #define v_Analog1 ???
  #define v_Analog2 ???
  #define VBATPIN ???
#elif ADAFRUIT_FEATHER_M0
  //Sortie du 1-Wire
  #define ONEWIRE_OUTPUT 5
  #define ledPin 13
  #define v_OneWire 12
  #define v_I2C 11
  #define v_Analog1 A4
  #define v_Analog2 A5
  #define VBATPIN A7
#endif

OneWire  ds(ONEWIRE_OUTPUT);

///////////////////////////////////////
//Déclaration des variables
//////////////////////////////////////
// Set how often alarm goes off here
const byte alarmSeconds = 3;
const byte alarmMinutes = 0;
const byte alarmHours = 0;

#define interruptPin A2

volatile bool alarmFlag = false; // Start awake
volatile bool buttonFlag = false;

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

static struct pt sensorAnalysisPT, sendingPT, receivingPT;
uint8_t tmpBuffer[SIZE_BUFFER];

void setup() {
  // Log terminal (high speed)
  //Serial.begin(115200);
  // Power for Serial1...
  pinMode(A3, OUTPUT);
  digitalWrite(A3, HIGH);
  Serial1.begin(9600);
  
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

  zerortc.begin(); // Set up clocks and such
  zerortc.setEpoch(0);
  zerortc.setAlarmEpoch(zerortc.getEpoch() + SENSOR_INTERVAL);
  zerortc.enableAlarm(zerortc.MATCH_HHMMSS);
  zerortc.attachInterrupt(alarmMatch); // Set up a handler for the alarm
  
  PT_INIT(&sensorAnalysisPT);
  PT_INIT(&sendingPT);
  PT_INIT(&receivingPT);
  if(true/*Serial1*/) {
    //On allume la LED car on est connecté au serial (mode debug)
    digitalWrite(ledPin, HIGH);
    Serial1.println("########################################");
    Serial1.println("########## Akuino Sensor v17 ##########");
    Serial1.println("########################################");
    Serial1.print(epochToTime());
    Serial1.print(";D;Fréquence:");
    Serial1.print(frequence_LoRa);
    Serial1.print("MHz");
    Serial1.print(";Bande passante=");
    Serial1.print(bande_passante_LoRa);
    Serial1.print("KHz");
    Serial1.print(";Puissance=");
    Serial1.print(puissance_signal_LoRa);
    Serial1.print("mW");
    Serial1.print(";ID=" );
    Serial1.println(SENSOR_ID);
  }
}

void loop() {
  if (alarmFlag == true) {
    alarmFlag = false;  // Clear flag
    Serial1.print(epochToTime());
    Serial1.println(";D;Alarm triggered!");
  }
  if (buttonFlag == true) {
    buttonFlag = false;  // Clear flag
    Serial1.print(epochToTime());
    Serial1.println(";D;Button changed");
  }
  Serial1.print(epochToTime());
  Serial1.println(";I;Awaken");
  digitalWrite(ledPin, HIGH);

  sensorAnalysis(&sensorAnalysisPT);
  loRaSending(&sendingPT);
  //receiving(&receivingPT);

  if(true/*Serial1*/) {
    Serial1.print(epochToTime());
    Serial1.println(";D;Sleeping Microchip");
  }
  Serial1.print(epochToTime());
  Serial1.println(";I;Alarm set, sleeping now");
  //On coupe toute les alimentation
  digitalWrite(ledPin, LOW);
  compteur++;
  Serial1.flush();
  digitalWrite(A3, LOW);
  zerortc.setAlarmEpoch(zerortc.getEpoch() + SENSOR_INTERVAL);
  zerortc.enableAlarm(zerortc.MATCH_HHMMSS);
  zerortc.standbyMode();    // Sleep until next alarm match
  digitalWrite(A3, HIGH);
  delay(10);
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

  delay(800); // maybe 750ms is enough, maybe not  // PROTOTHREAD
  // we might do a ds.depower() here, but the reset will take care of it.

  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE); // Read Scratchpad

  if(true/*Serial1*/) {
    Serial1.print(epochToTime());
    Serial1.print(";D;Data = ");
    Serial1.print(present, HEX);
    Serial1.print(" ");
  }
  for ( i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
    if (true/*Serial1*/) {
      Serial1.print(data[i], HEX);
      Serial1.print(";");
    }
  }
  if(true/*Serial1*/) {
    Serial1.print("CRC=");
    Serial1.print(OneWire::crc8(data, 8), HEX);
    Serial1.println();
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
  if (true/*Serial1*/) {
    Serial1.print(epochToTime());
    Serial1.print(";D;Temperature = ");
    Serial1.print(celsius);
    Serial1.print(" Celsius, ");
    Serial1.print(fahrenheit);
    Serial1.println(" Fahrenheit");
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
  if(true/*Serial1*/){
    Serial1.print(epochToTime());
    Serial1.print(";D;");
    Serial1.println(code_message);
  }
}

extern int sensorAnalysis(struct pt *pt) {
  PT_BEGIN(pt);
  /////////////////////////////////////////
  //Analyse et lecture des I²C disponibles
  ////////////////////////////////////////
  PT_WAIT_UNTIL(pt, (zerortc.getEpoch() >= (previousMillis + SENSOR_INTERVAL)));
  if (true/*Serial1*/) {Serial1.println("");}
  //Variable a envoyer via LoRa
  char buffer_a_envoyer[SIZE_BUFFER]="";
  //pourcentage de la batterie
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
  //On écrit le header(en-tête) du buffer_a_envoyer + Niveau de batterie
  //le "moins" permet d'écrire à gauche
  dtostrf(measuredvbat, -5, 3, v1_string);
  // 1st piece of data, no "&" before field code...
  snprintf(&buffer_a_envoyer[strlen(buffer_a_envoyer)], SIZE_BUFFER - strlen(buffer_a_envoyer), "B=%s", v1_string);

  ////////////////////////////////////////////////
  //Analyse et lecture des analogiques disponibles
  ////////////////////////////////////////////////
  int pinA0=0;
  int pinA1=0;
  int pinA2=0;

  pinA0=digitalRead(A0);
  pinA1=analogRead(A1);
  pinA2=analogRead(A2);

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
  if(true/*Serial1*/) {
    Serial1.print(epochToTime());
    Serial1.println(";I;Begin I²C analysis");
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
      if (true/*Serial1*/) {
        Serial1.print(epochToTime());
        Serial1.print(";I;Sensor found at 0x");
        Serial1.println(address,HEX);
      }
      nDevices++;

      //////////////////////////////////
      //SHT31 : Température et humidité
      //Adresse I²C: 0x44
      /////////////////////////////////
      if (true/*Serial1*/) {Serial1.print(epochToTime());}
      if(address == 0x44) { //Adresse en Hexadécimal
        if (! sht31.begin(0x44)) {
          if (true/*Serial1*/) {Serial1.println(";E;H1 or T1 not connected");}
        }
        else {
          //on lit les informations de ce capteur
          float t = sht31.readTemperature();
          float h = sht31.readHumidity();

          if (!isnan(t)) {  // check if 'is not a number'
            if (true/*Serial1*/) {
              Serial1.print(";I;t = ");
              Serial1.print(t);
            }
          } else {
            if (true/*Serial1*/) {Serial1.print(";W;t = null");}
          }
          if (!isnan(h)) {  // check if 'is not a number'
            if (true/*Serial1*/) {
              Serial1.print(";H = ");
              Serial1.println(h);
            }
          } else {
            if (true/*Serial1*/) {Serial1.println(";W;H = null");}
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
      if(true/*Serial1*/){
        Serial1.print(epochToTime());
        Serial1.print("Erreur inconnue à l'adresse 0x");
      }
      if (address<16)
      if(true/*Serial1*/) {
        Serial1.print(epochToTime());
        Serial1.print(";E;Unknown error at 0x");
        Serial1.println(address,HEX);
      }
    }
    digitalWrite(v_I2C, LOW);
  }
  if (true/*Serial1*/) {Serial1.print(epochToTime());}
  if (nDevices == 0) {
    if(true/*Serial1*/){Serial1.println(";E;No device found");}
  }
  else {
    if(true/*Serial1*/){Serial1.println(";I;End I²C analysis");}
  }
  ///////////////////////////////////////////////////////
  //Analyse et lecture des 1-Wire disponibles en cours...
  ///////////////////////////////////////////////////////

  if(true/*Serial1*/) {
    Serial1.print(epochToTime());
    Serial1.println(";I;Begin 1-Wire analysis");
  }
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8];

  if (true/*Serial1*/) {Serial1.print(epochToTime());}
  if (!ds.search(addr)) {
    if(true/*Serial1*/){Serial1.println(";E;No 1-Wire detected");}
    
    //if(true/*Serial1*/){Serial1.println("Recherche terminée");}
    //ds.reset_search();
    //delay(2500);
    //return;
  }
  else {
  //On récrit dans la variable du buffer
  snprintf(&buffer_a_envoyer[strlen(buffer_a_envoyer)], SIZE_BUFFER - strlen(buffer_a_envoyer), "&G=");
  if (true/*Serial1*/) {
    Serial1.print(epochToTime());
    Serial1.print(";D;ROM =");
  }
  for( i = 0; i < 8; i++) {
    Serial1.write(' ');
    if (addr[i]<16) {
      if (true/*Serial1*/) {Serial1.print("0");}
      //le "moins" permet d'écrire à gauche
      dtostrf(0, -0, 0, v1_string);
      //On récrit dans la variable du buffer
      snprintf(&buffer_a_envoyer[strlen(buffer_a_envoyer)], SIZE_BUFFER - strlen(buffer_a_envoyer), "%s", v1_string);
    }
    if(true/*Serial1*/){Serial1.print(addr[i], HEX);}
    //le "moins" permet d'écrire à gauche
    dtostrf(addr[i], -0, 0, v1_string);
    //On récrit dans la variable du buffer
    snprintf(&buffer_a_envoyer[strlen(buffer_a_envoyer)], SIZE_BUFFER - strlen(buffer_a_envoyer), "%s", v1_string);
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
    if(true/*Serial1*/){Serial1.println(";invalid CRC");}
  }
  if(true/*Serial1*/){Serial1.println();}

  if(true/*Serial1*/) {
    Serial1.print(epochToTime());
    Serial1.print(";D;CHIP ");
    Serial1.print(addr[0],HEX);
    //Le premier octet (ROM) indique quelle est le chip (dipositif)
    Serial1.print(" = ");
  }
  switch (addr[0]) {
    case 0x01:
      if(true/*Serial1*/){Serial1.println("DS1990 DS2401");}
      break;
    case 0x02:
      if(true/*Serial1*/){Serial1.println("DS1991");}
      break;
    case 0x04:
      if(true/*Serial1*/){Serial1.println("DS1994");}
      break;
    case 0x05:
      if(true/*Serial1*/){Serial1.println("DS2405");}
      break;
    case 0x06:
      if(true/*Serial1*/){Serial1.println("DS1992");}
      break;
    case 0x08:
      if(true/*Serial1*/){Serial1.println("DS1993");}
      break;
    case 0x0B:
      if(true/*Serial1*/){Serial1.println("DS1985");}
      break;
    case 0x10:
      if(true/*Serial1*/){Serial1.println("DS1820 DS18S20 DS1920");}
      break;
    case 0x12:
      if(true/*Serial1*/){Serial1.println("DS2406");}
      break;
    case 0x21:
      if(true/*Serial1*/){Serial1.println("DS1921");}
      break;
    case 0x22:
      if(true/*Serial1*/){Serial1.println("DS1822");}
      break;
    case 0x24:
      if(true/*Serial1*/){Serial1.println("DS1904");}
      break;
    case 0x28:
      if(true/*Serial1*/){Serial1.print("DS18B20;");}
      if(true/*Serial1*/){Serial1.println("Lecture des données de ce chip...");}
      //On appelle la fonction pour la lecture
      fonction_return_value = lecture_DS18B20(addr);
      //le "moins" permet d'écrire à gauche
      dtostrf(fonction_return_value, -5, 3, v1_string);
      //On récrit dans la variable du buffer
      snprintf(&buffer_a_envoyer[strlen(buffer_a_envoyer)], SIZE_BUFFER - strlen(buffer_a_envoyer), "/DS18B20=%s", v1_string);
      break;
    case 0x29:
      if(true/*Serial1*/){Serial1.println("DS2408");}
      break;
    case 0x36:
      if(true/*Serial1*/){Serial1.println("DS2740");}
      break;
    case 0x3B:
      if(true/*Serial1*/){Serial1.println("DS1825");}
      break;
    case 0x41:
      if(true/*Serial1*/){Serial1.println("DS1923");}
      break;
    default:
      if(true/*Serial1*/){Serial1.println(" n'est pas répertorié.");}
      break;
    }
  }
  if(true/*Serial1*/) {
    Serial1.print(epochToTime());
    Serial1.println(";I;End 1-Wire analysis");
  }
  previousMillis = zerortc.getEpoch();
  if(true/*Serial1*/) {
    Serial1.print(epochToTime());
    Serial1.println(";I;Scan end");
    Serial1.print(epochToTime());
    Serial1.print(";I;buffer = ");
    Serial1.println(buffer_a_envoyer);
  }
  // Serial peripheral (low speed)
  //Serial1.begin(9600);
  // MANAGE SERIAL READ...
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
  if(true/*Serial1*/) {
    Serial1.print(epochToTime());
    Serial1.println(";I;Data transmission via LoRa (RF95)");
  }
  
  if (true/*Serial1*/) {Serial1.print(epochToTime());}
  //Activation du LoRa
  if (!rhRDatagram.init()) {
    if(true/*Serial1*/){Serial1.println(";E;LoRa not initialized");}
  } else {
    if(true/*Serial1*/){Serial1.println(";I;LoRa initialized");}
  }
  if (true/*Serial1*/) {
    Serial1.print(epochToTime());
    Serial1.print(";I;FIFO size = ");
    Serial1.println(sensorFIFO.size());
  }
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  radioDriver.setFrequency(frequence_LoRa);
  radioDriver.setTxPower(puissance_signal_LoRa);
  radioDriver.setSignalBandwidth(bande_passante_LoRa);
  if (true/*Serial1*/) {Serial1.print(epochToTime());}
  if (!(sensorFIFO.isEmpty())) {
    sensorFIFO.peekBuffer(tmpBuffer, SIZE_BUFFER);
    JBCDIC jbcdic;
    uint8_t tmpBuffer2[SIZE_BUFFER];
    int counter = jbcdic.encode_to_jbcdic((char*)tmpBuffer, sensorFIFO.peek(), tmpBuffer2, SIZE_BUFFER);
    
    if(rhRDatagram.sendtoWait(tmpBuffer2, counter, SERVER_ADDRESS)) {
      sensorFIFO.popBuffer(tmpBuffer, SIZE_BUFFER);
      if (true/*Serial1*/) {
        Serial1.println(";I;Packet sent");
      }
      // Now wait for a reply from the server
    }
    else{
      if (true/*Serial1*/) {
        Serial1.println(";W;Packet not received");
      }
    }
    if (true/*Serial1*/) {Serial1.println("");}
  }
  PT_END(pt);
}

void EIC_ISR(void) {
  buttonFlag = true;  // toggle buttonFlag by XORing it against true
}

void alarmMatch(void)
{
  alarmFlag = true; // Set flag
}

uint32_t epochToTime(void) {
  uint32_t epochToSec = zerortc.getEpoch() - EPOCH_TIME;
  return epochToSec;
}
