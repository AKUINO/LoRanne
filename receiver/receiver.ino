#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RH_RF95.h>
#include <WiFiClientSecure.h>



Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

// OLED FeatherWing buttons map to different pins depending on board:
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


#define RFM95_RST     27   // "A"
#define RFM95_CS      14  //
#define RFM95_INT     32   //  next to A



///////////////////////////////////////
//Déclaration des variables
//////////////////////////////////////
//Fréquence d'utilisation (par defaut 869.35 MHz)
float frequence_LoRa = 869.35;
//Bande passante utilisée (par defaut : 62,5 KHz)
float bande_passante_LoRa = 62500;
//Puissance utilisée (par defaut : 10 mW)
float puissance_signal_LoRa = 10;

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ frequence_LoRa

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);



//////////////////////////////////////////////////////////

//WIFI

/*
	const char* ssid     = "WiFi-2.4-4F58";
	const char* password = "yTre5QK4pP35";
*/

const char* ssid     = "AndroidAP";
const char* password = "badrdibdib";


const char*  server = "http://phenics.gembloux.ulg.ac.be";  // Server URL

// www.howsmyssl.com root certificate authority, to verify the server
// change it to your server root CA
// SHA1 fingerprint is broken now!


const char* test_root_ca= \
"";




//certificat SSL LOCAL
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



// You can use x.509 client certificates if you want
//const char* test_client_key = "";   //to verify the client
//const char* test_client_cert = "";  //to verify the client


WiFiClientSecure client;


//////////////////////////////////////////////////////////




//Taille FIXE Maximale du buffer d'envoi LoRa
uint8_t SIZE_URL_HTTPS = 200;
//Variable a envoyer via LoRa
char URL_HTTPS[200];

String URL;


void setup(){
    Serial.begin(9600);
	
	
	//////////////////////////
	
	Serial.println("OLED FeatherWing test");
	// SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32
	
	Serial.println("OLED begun");
	
	// Show image buffer on the display hardware.
	// Since the buffer is intialized with an Adafruit splashscreen
	// internally, this will display the splashscreen.
	display.display();
	delay(1000);
	
	
	Serial.println("IO test");
	
	pinMode(BUTTON_A, INPUT_PULLUP);
	pinMode(BUTTON_B, INPUT_PULLUP);
	pinMode(BUTTON_C, INPUT_PULLUP);
	
	
	/*
		display.clearDisplay();
		display.display();
		
		
		// text display tests
		display.setTextSize(1);
		display.setTextColor(WHITE);
		display.setCursor(0,0);
		display.print("Connecting to SSID\n'adafruit':");
		display.print("connected!");
		display.println("IP: 10.0.1.23");
		display.println("Sending val #0");
		display.setCursor(0,0);
		display.display(); // actually display all of the above
		
	*/
	
	//////////////////////////
	
	pinMode(RFM95_RST, OUTPUT);
	digitalWrite(RFM95_RST, HIGH);
	
	Serial.begin(9600);
	
	/*
		while (!Serial) {
		delay(1);
		}
	*/
	
	delay(100);
	
	
	
	Serial.println("Initialisation du LoRa...");
	
	// manual reset
	digitalWrite(RFM95_RST, LOW);
	delay(10);
	digitalWrite(RFM95_RST, HIGH);
	delay(10);
	
	while (!rf95.init()) {
		Serial.println("LoRa radio init failed");
		while (1);
	}
	Serial.println("LoRa radio init OK!");
	
	
	rf95.setFrequency(frequence_LoRa);
	rf95.setTxPower(puissance_signal_LoRa);
	rf95.setSignalBandwidth(bande_passante_LoRa);
	
	delay(1000);
	
	
	
	
	
	
	
	
    
    if(!SD.begin(33)){
        Serial.println("Card Mount Failed");
        return;
	}
    uint8_t cardType = SD.cardType();
	
    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
	}
	
    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
		} else if(cardType == CARD_SD){
        Serial.println("SDSC");
		} else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
		} else {
        Serial.println("UNKNOWN");
	}
	
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
	
	
	/*
		listDir(SD, "/", 0);
		createDir(SD, "/mydir");
		listDir(SD, "/", 0);
		removeDir(SD, "/mydir");
		listDir(SD, "/", 2);
		writeFile(SD, "/hello.txt", "Hello ");
		appendFile(SD, "/hello.txt", "World!\n");
		readFile(SD, "/hello.txt");
		deleteFile(SD, "/foo.txt");
		renameFile(SD, "/hello.txt", "/foo.txt");
		readFile(SD, "/foo.txt");
		testFileIO(SD, "/test.txt");
		Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
		Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
		
		
	*/
	
	
	
	
	
	
	WiFi.begin(ssid, password); 
	
	while (WiFi.status() != WL_CONNECTED) {
		delay(1000);
		Serial.print("Connecting to WiFi..");
		Serial.println(ssid);
	}
	
	Serial.println("Connected to the WiFi network");
	
	
	
	client.setCACert(test_root_ca2);
	
	
	
	Serial.println("On démarre ecoute...");
	
}

void loop()
{
	
	//Variable utilisée pour la récriture du buffer_a_envoyer
	char v1_string[5];
	
	
	if (rf95.available())
	{
		// Should be a message for us now
		uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
		uint8_t len = sizeof(buf);
		
		if (rf95.recv(buf, &len))
		{
			
			RH_RF95::printBuffer("Données recues [HEXA]: ", buf, len);
			Serial.print("Données recues [Texte]: ");
			Serial.println((char*)buf);
			Serial.print("RSSI: ");
			Serial.println(rf95.lastRssi(), DEC);
			
			
			
			
			//On envoie les données sur le serveur HTTP
			
			
			
			
			
			display.clearDisplay();
			display.display();
			// text display tests
			display.setTextSize(1);
			display.setTextColor(WHITE);
			display.setCursor(0,0);
			display.println("MESSAGE RECU :");
			display.print((char*)buf);
			display.setCursor(0,0);
			display.display(); // actually display all of the above
			
			
			//
			int16_t RSSI_SENSOR = rf95.lastRssi()*-1;
			
			
			
			//const char* host = "192.168.1.43";
			
			const char* host = "192.168.43.138";
			unsigned long timeout = millis();
			WiFiClient client;
			const int httpPort = 80;
			
			snprintf(URL_HTTPS, SIZE_URL_HTTPS, "http://192.168.43.138/AKUINO_Dashboard/app/reception.php?v=-%u&%s", RSSI_SENSOR, (char*)buf);
			
			Serial.println(URL_HTTPS);
			
			
			
			URL=String(URL_HTTPS);
			
			//Adaptation du format de l'url pour mon serveur (dashboard personalisé)
			
			
			URL.replace("R=", "");
			URL.replace("&M=", "/");
			URL.replace("&B=", "/");
			URL.replace("&", "/");
			
            // Use WiFiClient class to create TCP connections
			
			
			
			/*
				if (!client.connect(host, httpPort)) {
				Serial.println("http connection failed");
				return;
				}
				
				Serial.print("Requesting URL: ");
				Serial.println(URL_HTTPS);
				
				// This will send the request to the server
				client.print(String("GET ") + URL + " HTTP/1.1\r\n" +
				"Host: " + host + "\r\n" +
				"Connection: close\r\n\r\n");
				
				while (client.available() == 0) {
				if (millis() - timeout > 5000) {
				Serial.println(">>> Client Timeout !");
				client.stop();
				return;
				}
				}
				
				// Read all the lines of the reply from server and print them to Serial
				while(client.available()) {
				String line = client.readStringUntil('\r');
				Serial.print(line);
				}
				
				
			*/
			
			
			
			///////////////////////////////////////////
			//On envoie au serveur ELSA
			//////////////////////////////////////////
			
			
			host = "phenics.gembloux.ulg.ac.be";
			
			snprintf(URL_HTTPS, SIZE_URL_HTTPS, "/api/kv?R=-%u&%s", RSSI_SENSOR, (char*)buf);
			
			Serial.println(URL_HTTPS);
			
			
			URL=String(URL_HTTPS);
			
			
			
            // Use WiFiClient class to create TCP connections
			
			if (!client.connect(host, httpPort)) {
				Serial.println("http connection failed");
				return;
			}
			
			Serial.print("Requesting URL: ");
			Serial.println(URL_HTTPS);
			
			// This will send the request to the server
			client.print(String("GET ") + URL + " HTTP/1.1\r\n" +
			"Host: " + host + "\r\n" +
			"Connection: close\r\n\r\n");
			
			while (client.available() == 0) {
				if (millis() - timeout > 5000) {
					Serial.println(">>> Client Timeout !");
					client.stop();
					return;
				}
			}
			
			// Read all the lines of the reply from server and print them to Serial
			while(client.available()) {
				String line = client.readStringUntil('\r');
				Serial.print(line);
			}
			
			
			
			
			
			
			//
			//On écrit sur la carte SD
			
			
			//    appendFile(SD, "/sauvegarde.txt", (char*)buf);
			
			/*
				// Send a reply
				uint8_t data[] = "J'ai bien recu les données!! :)";
				rf95.send(data, sizeof(data));
				rf95.waitPacketSent();
				Serial.println("Envoie d'une réponse!");
			*/
			
			
			
			//readFile(SD, "/hello.txt");
			
			
			
		}
		else
		{
			Serial.println("Receive failed");
		}
	}
	
	
	
	
	
	
	
	
}










void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);
	
    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
	}
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
	}
	
    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
			}
			} else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
		}
        file = root.openNextFile();
	}
}

void createDir(fs::FS &fs, const char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
		} else {
        Serial.println("mkdir failed");
	}
}

void removeDir(fs::FS &fs, const char * path){
    Serial.printf("Removing Dir: %s\n", path);
    if(fs.rmdir(path)){
        Serial.println("Dir removed");
		} else {
        Serial.println("rmdir failed");
	}
}

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);
	
    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
	}
	
    Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
	}
    file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);
	
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
	}
    if(file.print(message)){
        Serial.println("File written");
		} else {
        Serial.println("Write failed");
	}
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);
	
    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
	}
    if(file.println(message)){
        Serial.println("Message appended");
		} else {
        Serial.println("Append failed");
	}
    file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
		} else {
        Serial.println("Rename failed");
	}
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
		} else {
        Serial.println("Delete failed");
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
        Serial.printf("%u bytes read for %u ms\n", flen, end);
        file.close();
		} else {
        Serial.println("Failed to open file for reading");
	}
	
	
    file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
	}
	
    size_t i;
    start = millis();
    for(i=0; i<2048; i++){
        file.write(buf, 512);
	}
    end = millis() - start;
    Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
    file.close();
}