#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <Wire.h>
#include "pt.h"
#include "FIFO.h"
// We are not encoding JSON but URL ...
#include "JBCDIC.h"

// LoRa Network Address
#define SERVER_ADDRESS 120

#ifdef ARDUINO_SAMD_MKRWAN1300
  // no screen, no Wifi but LoRa
  #include "RH_RF95.h"
  #include "RHReliableDatagram.h"
#elif ARDUINO_FEATHER_ESP32
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>
  #include "RTClib.h"
  #include <WiFiClientSecure.h>
  #include <WiFiClient.h>
  #include "RH_RF95.h"
  #include "RHReliableDatagram.h"
  #include "IotWebConf.h"

  // When stacking ADAFruit boards (ESP32, OLED, RTC+SD card, LoRa board), PCF8523 is the RTC used
  RTC_PCF8523 rtc;
  Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

  #define RFM95_RST     27
  #define RFM95_CS      14 
  #define RFM95_INT     32

  // Singleton instance of the radio driver
  RH_RF95 radioDriver(RFM95_CS, RFM95_INT);
  
  RHReliableDatagram rhRDatagram(radioDriver, SERVER_ADDRESS);
#elif ARDUINO_M5Stack_Core_ESP32
  // No LoRa and Another type of screen...
  #include <WiFiClientSecure.h>
  #include <WiFiClient.h>
  #include "IotWebConf.h"
#endif

// PHENICS is not supporting HTTPS
#undef __HTTPS__

// OLED FeatherWing buttons map to different pins depending on board:x  
#if defined(ESP8266)
  #define BUTTON_A  0
  #define BUTTON_B 16
  #define BUTTON_C  2
#elif defined(ESP32)
  #define BUTTON_A 15
  #define BUTTON_B 32
  #define BUTTON_C 14
#elif defined(ARDUINO_STM32_FEATHER)
  #define BUTTON_A PA15
  #define BUTTON_B PC7
  #define BUTTON_C PC5
#elif defined(TEENSYDUINO)
  #define BUTTON_A  4
  #define BUTTON_B  3
  #define BUTTON_C  8
#elif defined(ARDUINO_FEATHER52832)
  #define BUTTON_A 31
  #define BUTTON_B 30
  #define BUTTON_C 27
#else // 32u4, M0, M4, nrf52840 and 328p
  #define BUTTON_A  9
  #define BUTTON_B  6
  #define BUTTON_C  5
#endif

//TODO: For Arduino MKR ?
// ARDUINO_STM32_FEATHER
#define CONFIG_PIN 15
#define STATUS_PIN 13

// IotWebConf:
#define HTTP_PORT 80
#define STRING_LEN 128
#define CONFIG_VERSION "dem2"

// LoRa:
//Fréquence d'utilisation (par defaut 869.35 MHz)
float frequence_LoRa = 869.35;
//Bande passante utilisée (par defaut : 62,5 KHz)
long bande_passante_LoRa = 62500;
//Puissance utilisée (par defaut : 10 mW)
uint8_t puissance_signal_LoRa = 10;

//Certificat SSL LOCAL
const char* test_root_ca2= \
"-----BEGIN CERTIFICATE-----\n" \
"MIIGfTCCBWWgAwIBAgIQKr426ykpq7kEPBdeDeXrWjANBgkqhkiG9w0BAQsFADBy\n" \
"MQswCQYDVQQGEwJVUzELMAkGA1UECBMCVFgxEDAOBgNVBAcTB0hvdXN0b24xFTAT\n" \
"BgNVBAoTDGNQYW5lbCwgSW5jLjEtMCsGA1UEAxMkY1BhbmVsLCBJbmMuIENlcnRp\n" \
"ZmljYXRpb24gQXV0aG9yaXR5MB4XDTE5MDIwNDAwMDAwMFoXDTE5MDUwNTIzNTk1\n" \
"OVowIzEhMB8GA1UEAxMYeGF2aWVyLW1hcnRpbi10cmFkaW5nLmZyMIIBIjANBgkq\n" \
"hkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAw6fc7jpyPqT7m6P2ltdSMfWfy0NSBKWh\n" \
"bHrSFPX8agMwn6T1+9yV1B8HIXkOTx7oX4DIrL2d8rYG13Rp2EhT2+Uomdq8AeXk\n" \
"TI8GHqlF76+KKpfKFsKMWSPVZcZJ3MMvP0g4g0mblnRy6iq6e/DJHQfh3aGFHYlP\n" \
"+uda1a2eZ85Utnz6nFEOufYEvDkynn53MWEuWT94ALdSn2E3f6IRGW2SjRi9fQ5N\n" \
"PzYeLfPlcjoqpZOkuQmbNv9gRu1/2x9iWQCLRnv6/Aphfd794YgveHuFZXN3PNa6\n" \
"BFuxzdNuPokWFU1PXIvlo8X9oVpLA6uiJhZjNK8HG/eF7tDJnaxvTQIDAQABo4ID\n" \
"XDCCA1gwHwYDVR0jBBgwFoAUfgNaZUFrp34K4bidCOodjh1qx2UwHQYDVR0OBBYE\n" \
"FGFSDqIWVkWTgpP1EQXIOnNuDmb5MA4GA1UdDwEB/wQEAwIFoDAMBgNVHRMBAf8E\n" \
"AjAAMB0GA1UdJQQWMBQGCCsGAQUFBwMBBggrBgEFBQcDAjBPBgNVHSAESDBGMDoG\n" \
"CysGAQQBsjEBAgI0MCswKQYIKwYBBQUHAgEWHWh0dHBzOi8vc2VjdXJlLmNvbW9k\n" \
"by5jb20vQ1BTMAgGBmeBDAECATBMBgNVHR8ERTBDMEGgP6A9hjtodHRwOi8vY3Js\n" \
"LmNvbW9kb2NhLmNvbS9jUGFuZWxJbmNDZXJ0aWZpY2F0aW9uQXV0aG9yaXR5LmNy\n" \
"bDB9BggrBgEFBQcBAQRxMG8wRwYIKwYBBQUHMAKGO2h0dHA6Ly9jcnQuY29tb2Rv\n" \
"Y2EuY29tL2NQYW5lbEluY0NlcnRpZmljYXRpb25BdXRob3JpdHkuY3J0MCQGCCsG\n" \
"AQUFBzABhhhodHRwOi8vb2NzcC5jb21vZG9jYS5jb20wgbIGA1UdEQSBqjCBp4IY\n" \
"eGF2aWVyLW1hcnRpbi10cmFkaW5nLmZygh1tYWlsLnhhdmllci1tYXJ0aW4tdHJh\n" \
"ZGluZy5mcoIod3d3Lnhhdmllci1tYXJ0aW4tdHJhZGluZy5kYXRhY2hldmV1LmNv\n" \
"bYIcd3d3Lnhhdmllci1tYXJ0aW4tdHJhZGluZy5mcoIkeGF2aWVyLW1hcnRpbi10\n" \
"cmFkaW5nLmRhdGFjaGV2ZXUuY29tMIIBBAYKKwYBBAHWeQIEAgSB9QSB8gDwAHYA\n" \
"u9nfvB+KcbWTlCOXqpJ7RzhXlQqrUugakJZkNo4e0YUAAAFotdADvwAABAMARzBF\n" \
"AiAv0NCyIgZ2cHpI+jClurKHwtSNZWXiXcU64fytrKsO5QIhAJ2QK9BfoUbjowwH\n" \
"1BF32g5spEbii9CyWjKD2WD4qstfAHYAdH7agzGtMxCRIZzOJU9CcMK//V5CIAjG\n" \
"NzV55hB7zFYAAAFotdAEFAAABAMARzBFAiAaCKsJw3yo6yuLIsz1E5v6XwkuRT4Q\n" \
"qSDYl+1JW8pz4gIhAIxbIIb1K81EQAaE3m8P/1C4K8dG+sb4pHv8t0Qvd61wMA0G\n" \
"CSqGSIb3DQEBCwUAA4IBAQAthmGhMZXzRFpagH7+xPsxFbDEddxLOcLjU9svff9q\n" \
"GWXjK0PhzFjAQfFomzRkhcu3w4sVy0A0DcmMkAcAHi2H3KMlqT7xMpa4CE/hfuZk\n" \
"eJ4V4AXqiTJydUafvfYhQlNO3UnU68lmo18J4PgUw19Urxu2jERyrzsY4usqpvgb\n" \
"7Qw8ZJxK2gR/RYJKec74opj48O4t8EjPgONM9/SKVyQoNZFcUgBiwoylQ9yHrOpm\n" \
"M9uDLnGHxSL11zBUGkjX3Ym/Zxcf4sa+TeDWrt0F89PTGvChMK1rj54hbTkEaodA\n" \
"/A3hZ+wpT3sK6uWF/fenyZNt4/mgxoSbkYS/uaI5HSMD\n" \
"-----END CERTIFICATE-----\n";

#if __HTTPS__
WiFiClientSecure client;
#else
WiFiClient client;
#endif

//Taille FIXE Maximale du buffer d'envoi LoRa
#define SIZE_URL_HTTPS 200
//Variable a envoyer via LoRa
char URL_HTTPS[SIZE_URL_HTTPS];

static struct pt receivingPT, sendDataPT;

const char thingName[] = "Akuino";
const char wifiInitialApPassword[] = "12345678";

char hostServer[STRING_LEN];
boolean formValidator();
void configSaved();

DNSServer dnsServer;
WebServer server(80);

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);

FIFO serverfifo;

char daysOfTheWeek[7][12] = {"Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"};

void setup() {
  Serial.begin(115200);
  
  iotWebConf.setStatusPin(STATUS_PIN);
  iotWebConf.setConfigPin(CONFIG_PIN);
  iotWebConf.setConfigSavedCallback(&configSaved);
  iotWebConf.setFormValidator(&formValidator);

  iotWebConf.init();

  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });

//TODO: For Arduino MKR ?
  if (! rtc.begin()) {
    if(Serial) {
      Serial.print(millis());
      Serial.println(";E;Couldn't find RTC");
    }
  }

  if (! rtc.initialized()) {
    if(Serial) {
      Serial.print(millis());
      Serial.println(";E;RTC not running!");
    }
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

//TODO: For Arduino MKR ?
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32
  display.display();
  delay(1000);

//TODO: For Arduino MKR ?
//#ifdef ARDUINO_SAMD_MKRWAN1300
//  pinMode(LORA_IRQ_DUMB, OUTPUT);
//  digitalWrite(LORA_IRQ_DUMB, LOW);
//
//  // Hardware reset
//  pinMode(LORA_BOOT0, OUTPUT);
//  digitalWrite(LORA_BOOT0, LOW);
//
//  pinMode(LORA_RESET, OUTPUT);
//  digitalWrite(LORA_RESET, HIGH);
//  delay(200);
//  digitalWrite(LORA_RESET, LOW);
//  delay(200);
//  digitalWrite(LORA_RESET, HIGH);
//  delay(50);
//#endif
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  delay(100);

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  
  delay(10);
  while (!rhRDatagram.init()) {
    if(Serial) {
      Serial.print(millis());
      Serial.println(";E;LoRa init error");
    }
  }
  if(Serial) {
    Serial.print(millis());
    Serial.println(";I;LoRa initialized");
  }

  radioDriver.setFrequency(frequence_LoRa);
  radioDriver.setTxPower(puissance_signal_LoRa);
  radioDriver.setSignalBandwidth(bande_passante_LoRa);

  delay(1000);

  PT_INIT(&receivingPT);
  PT_INIT(&sendDataPT);
  
//TODO: For Arduino MKR ?
  if(!SD.begin(33)){
    if(Serial) {
      Serial.print(millis());
      Serial.println(";E;Card assembly failure"); //Echec du montage de la carte
    }
    return;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    if(Serial) {
      Serial.print(millis());
      Serial.println(";I;No SD card");
    }
    return;
  }

  if(Serial) {
    Serial.print(millis());
    Serial.print(";I;SD type : ");
  }
  if(cardType == CARD_MMC){
    if(Serial) {Serial.println("MMC");}
  } else if(cardType == CARD_SD){
    if(Serial) {Serial.println("SDSC");}
  } else if(cardType == CARD_SDHC){
    if(Serial) {Serial.println("SDHC");}
  } else {
    if(Serial) {Serial.println("UNKNOWN");}
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  if(Serial) {
    Serial.print(millis());
    Serial.printf(";SD storage size : %lluMB\n", cardSize);
  }
}

void loop() {
  iotWebConf.doLoop();
  receiving(&receivingPT);
  sendData(&sendDataPT);
}

void handleRoot() {
  if (iotWebConf.handleCaptivePortal())
  {
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>IotWebConf 03 Custom Parameters</title>";
  s += "<style>.de{background-color:#ffaaaa;} .em{font-size:0.8em;color:#bb0000;padding-bottom:0px;} .c{text-align: center;} div,input{padding:5px;font-size:1em;} input{width:95%;} body{text-align: center;font-family:verdana;} button{border:0;border-radius:0.3rem;background-color:#16A1E7;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;} fieldset{border-radius:0.3rem;margin: 0px;}</style>";
  s += "<script>function c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();}";
  s += "</script></head><body>";
  s += "Welcome on the Akuino configuration home page.<br /><br />";
  s += "Go to <a href='config'>configure page</a> to change values.<br /><br />";
  s += "Current values : <br /><br />";
  s += "<div style='text-align:left;display:inline-block;min-width:260px;'>";
  s += "<form action='' method='post'><fieldset><input type='hidden' name='iotSave' height=100 value='true'>";
  s += "<div class='class'><label>Thing Name</label><br /><input value=";
  s += iotWebConf.getThingName();
  s += "></div>";
  s += "<div class='class'><label>WiFi SSID</label><br /><input value=";
  s += iotWebConf.getWifiSsid();
  s += "></div>";
  s += "<div class='class'><label>Host</label><br /><input value=";
  s += iotWebConf.getHostName();
  s += "></div>";
  s += "</fieldset></form>";
  s += "</div>";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
}

void configSaved() {
  if(Serial) {
    Serial.print(millis());
    Serial.println(";I;Configuration updated");
  }
}

boolean formValidator() {
  if(Serial) {
    Serial.print(millis());
    Serial.print(";D;Validating form");
  }

  return true;
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  if(Serial) {
    Serial.print(millis());
    Serial.printf(";I;Listing directory: %s\n", dirname);
  }
  File root = fs.open(dirname);
  if(!root){
    if(Serial) {
      Serial.print(millis());
      Serial.println(";W;Failed to open directory");
    }
    return;
  }
  if(!root.isDirectory()){
    if(Serial) {
      Serial.print(millis());
      Serial.println(";W;Not a directory");
    }
    return;
  }

  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      if(Serial) {
        Serial.print(millis());
        Serial.print(";D;DIR : ");
        Serial.println(file.name());
      }
      if(levels){
        listDir(fs, file.name(), levels -1);
      }
    } else {
      if(Serial) {
        Serial.print(millis());
        Serial.print(";D;FILE : ");
        Serial.print(file.name());
        Serial.print(";SIZE : ");
        Serial.println(file.size());
      }
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char * path){
  if(Serial) {
    Serial.print(millis());
    Serial.printf(";I;Creating Dir: %s\n", path);
  }
  if(fs.mkdir(path)){
    if(Serial) {
      Serial.print(millis());
      Serial.println(";D;Dir created");
    }
  } else {
    if(Serial) {
      Serial.print(millis());
      Serial.println(";W;mkdir failed");
    }
  }
}

void removeDir(fs::FS &fs, const char * path){
  if(Serial) {
    Serial.print(millis());
    Serial.printf(";I;Removing Dir: %s\n", path);
  }
  if(fs.rmdir(path)){
    if(Serial) {
      Serial.print(millis());
      Serial.println(";D;Dir removed");
    }
  } else {
    if(Serial) {
      Serial.print(millis());
      Serial.println(";W;rmdir failed");
    }
  }
}

void readFile(fs::FS &fs, const char * path){
  if(Serial) {
    Serial.print(millis());
    Serial.printf(";I;Reading file: %s\n", path);
  }
  File file = fs.open(path);
  if(!file){
    if(Serial) {
      Serial.print(millis());
      Serial.println(";W;Failed to open file for reading");
    }
    return;
  }
  if(Serial) {
    Serial.print(millis());
    Serial.print(";D;Read from file: ");
  }
  while(file.available()){
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  if(Serial) {
    Serial.print(millis());
    Serial.printf(";I;Writing file: %s\n", path);
  }
  File file = fs.open(path, FILE_WRITE);
  if(!file){
    if(Serial) {
      Serial.print(millis());
      Serial.println(";W;Failed to open file for writing");
    }
    return;
  }
  if(file.print(message)){
    if(Serial) {
      Serial.print(millis());
      Serial.println(";D;File written");
    }
  } else {
    if(Serial) {
      Serial.print(millis());
      Serial.println(";W;Write failed");
    }
  }
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
  if(Serial) {
    Serial.print(millis());
    Serial.printf(";I;File writing (micro-SD) : %s\n", path);
  }
  File file = fs.open(path, FILE_APPEND);
  if(!file){
    if(Serial) {
      Serial.print(millis());
      Serial.println(";W;File opening failure");
    }
    return;
  }
  if(file.println(message)){
    if(Serial) {
      Serial.print(millis());
      Serial.println(";D;Datas added to file");
    }
  } else {
    if(Serial) {
      Serial.print(millis());
      Serial.println(";E;Error");// Erreur ! :(
    }
  }
  file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
  if(Serial) {
    Serial.print(millis());
    Serial.printf(";I;Renaming file %s to %s\n", path1, path2);
  }
  if (fs.rename(path1, path2)) {
    if(Serial) {
      Serial.print(millis());
      Serial.println(";D;File renamed");
    }
  } else {
    if(Serial) {
      Serial.print(millis());
      Serial.println(";W;Rename failed");
    }
  }
}

void deleteFile(fs::FS &fs, const char * path){
  if(Serial) {
    Serial.print(millis());
    Serial.printf(";I;Deleting file: %s\n", path);
  }
  if(fs.remove(path)){
    if(Serial) {
      Serial.print(millis());
      Serial.println(";D;File deleted");
    }
  } else {
    if(Serial) {
      Serial.print(millis());
      Serial.println(";W;Delete failed");
    }
  }
}

void testFileIO(fs::FS &fs, const char * path){
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if(file){
    len = file.size();
    size_t flen = len;
    start = millis();
    while(len){
      size_t toRead = len;
      if(toRead > 512){
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    if(Serial) {
      Serial.print(millis());
      Serial.printf(";I;%u bytes read for %u ms\n", flen, end);
    }
    file.close();
  } else {
    if(Serial) {
      Serial.print(millis());
      Serial.println(";W;Failed to open file for reading");
    }
  }

  file = fs.open(path, FILE_WRITE);
  if(!file){
    if(Serial) {
      Serial.print(millis());
      Serial.println(";W;Failed to open file for writing");
    }
    return;
  }
  size_t i;
  start = millis();
  for(i=0; i<2048; i++){
    file.write(buf, 512);
  }
  end = millis() - start;
  if(Serial) {
    Serial.print(millis());
    Serial.printf(";D;%u bytes written for %u ms\n", 2048 * 512, end);
  }
  file.close();
}

extern int receiving(struct pt *pt) {
  PT_BEGIN(pt);
//  PT_WAIT_UNTIL(pt, (1 == 1));
  if (rhRDatagram.available()) {
    // Should be a message for us now
    char buf2[SIZE_URL_HTTPS];
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = RH_RF95_MAX_MESSAGE_LEN;
    uint16_t timeout = 5000;
    uint8_t from;
    //rhRDatagram.waitAvailable();
    if (rhRDatagram.recvfromAckTimeout(buf, &len, timeout, &from)) {
      if(Serial) {
        Serial.print(millis());
        Serial.println(";I;Message received");
      }
      buf[len] = '\0';
      int sz = JBCDIC::decode_from_jbcdic(buf, len, buf2, SIZE_URL_HTTPS);
      buf2[sz] = '\0';
      if(Serial) {
        Serial.print(millis());
        Serial.print(";I;");
      }
      RH_RF95::printBuffer("Données recues [HEXA]: ", (uint8_t *)buf2, sz);
      if(Serial) {
        Serial.print(millis());
        Serial.print(";I;Received datas [Texte]: ");
        for(int i =0; i < sz; i++){
            Serial.print(buf2[i]); 
        }
        Serial.print(";RSSI: ");
        Serial.println(radioDriver.lastRssi(), DEC);
      }

      DateTime now = rtc.now();

      if(Serial) {
        Serial.print(millis());
        Serial.print(";D;Date/RTC Time : ");
        Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
        Serial.print(' ');
        Serial.print(now.day(), DEC);
        Serial.print('/');
        Serial.print(now.month(), DEC);
        Serial.print('/');
        Serial.print(now.year(), DEC);
        Serial.print(' ');
        Serial.print(now.hour(), DEC);
        Serial.print(':');
        Serial.print(now.minute(), DEC);
        Serial.print(':');
        Serial.print(now.second(), DEC);
        Serial.println();
      }
      
//      if(rhRDatagram.sendtoWait((uint8_t *)buf2, sz, from)) {
//        if(Serial) {
//          Serial.print(millis());
//          Serial.print(";D;Response sent");
//          Serial.println(";Saving data on micro-SD");
//        }
//      }
      appendFile(SD, "/sauvegarde.txt", buf2);
      
      display.clearDisplay();
      display.display();
      // text display tests
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.println("MESSAGE RECU :");
      display.print(buf2);
      display.setCursor(0,0);
      display.display(); // actually display all of the above
      
      int16_t RSSI_sensor = radioDriver.lastRssi()*-1;
      if(Serial) {
        Serial.print(millis());
        Serial.print(";D;Host : ");
        Serial.println(hostServer );
      }
      unsigned long ttl = millis();

      ///////////////////////////////////////////
      //On envoie au serveur ELSA
      //////////////////////////////////////////
      int count = snprintf(URL_HTTPS, SIZE_URL_HTTPS, "%d&M=%u&%s", RSSI_sensor, from, buf2);
      
      Serial.print(millis());
      if(serverfifo.pushBuffer((uint8_t*)URL_HTTPS,count)){
        Serial.print(";I;Add in FIFO  size : ");
        Serial.println(serverfifo.size()); 
      }
      else{
         Serial.println(";I;FIFO FULL");
      }
      
      if(Serial) {
        Serial.print(millis());
        Serial.print(";D;URL HTTPS : ");
        Serial.println(URL_HTTPS);
      }
    }
    else {
      if(Serial) {
        Serial.print(millis());
        Serial.println(";D;Reception failed");
      }
    }
  }
  PT_END(pt);
}

extern int sendData(struct pt *pt) {
  PT_BEGIN(pt);
  PT_WAIT_UNTIL(pt, (!serverfifo.isEmpty()));
  //On envoie les données sur le serveur HTTP
  if(Serial) {
    Serial.print(millis());
    Serial.println(";I;Sending data on both servers (ELSA + SERVEUR LOCAL)");
  }
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  char buf2[SIZE_URL_HTTPS];
  
  serverfifo.peekBuffer(buf, RH_RF95_MAX_MESSAGE_LEN);
  int count = snprintf(buf2, SIZE_URL_HTTPS, "/api/kv?R=%s", (char*)buf);

  // Use WiFiClient class to create TCP connections

  strncpy(hostServer, iotWebConf.getHostName(), STRING_LEN);
#ifdef __HTTPS__
  client.setCACert(test_root_ca2);
#endif
  if (!client.connect(hostServer , HTTP_PORT)) {
    if(Serial) {
      Serial.print(millis());
      Serial.println(";W;HTTP connection failed");
    }
  }
  else{
    if(Serial) {
      Serial.print(millis());
      Serial.print(";D;Requesting URL: ");
      Serial.println(URL_HTTPS);
    }

    // This will send the request to the server
    client.print("GET ");
    client.print(buf2);
    client.print(" HTTP/1.1\r\n");
    client.print("Host: ");
    client.print(hostServer);
    client.print("\r\nConnection: close\r\n\r\n");

    boolean error = false;
    int timeout = millis();
    while (client.available() == 0 && !error) {
      if ((millis() - timeout) > 5000) {
        if(Serial) {
          Serial.print(millis());
          Serial.println(";W;Web Application Timeout");
        }
        client.stop();
        error = true;
      }
    }
      
    if(Serial && !error){
      Serial.print(millis());
      Serial.print(";I;Server response");
      serverfifo.popBuffer(buf, RH_RF95_MAX_MESSAGE_LEN);
    }
    
    // Read all the lines of the reply from server and print them to Serial
    while(client.available()) {
      String line = client.readStringUntil('\r');
      if(Serial) {Serial.print(line);}
    }
  }
  PT_END(pt);
}
