/*
          _          _                   _____                           
    /\   | |        (_)                 / ____|                          
   /  \  | | ___   _ _ _ __   ___ _____| (___   ___ _ __  ___  ___  _ __ 
  / /\ \ | |/ / | | | | '_ \ / _ \______\___ \ / _ \ '_ \/ __|/ _ \| '__|
 / ____ \|   <| |_| | | | | | (_) |     ____) |  __/ | | \__ \ (_) | |   
/_/    \_\_|\_\\__,_|_|_| |_|\___/     |_____/ \___|_| |_|___/\___/|_|   

Programme de check-up + envoie des données sur le GateWay via LoRa

*/


//id du sensor
char SENSOR_ID[]="M=5";

#include <Wire.h>
#include <OneWire.h>
#include <SPI.h>
#include <avr/dtostrf.h>
#include "Adafruit_SHT31.h"

Adafruit_SHT31 sht31 = Adafruit_SHT31();

#include <RH_RF95.h>
RH_RF95 rf95(8, 3);



/////////////////////////////
//Déclaration des sorties
/////////////////////////////

//Sortie du 1-Wire
OneWire  ds(5);



///////////////////////////////////////
//Déclaration des variables
//////////////////////////////////////
//Fréquence d'utilisation (par defaut 869.35 MHz)
float frequence_LoRa = 869.35;
//Bande passante utilisée (par defaut : 62,5 KHz)
float bande_passante_LoRa = 62500;
//Puissance utilisée (par defaut : 10 mW)
float puissance_signal_LoRa = 10;

//Fréquence d'échantillonnage en min (par defaut : 15min)
float frequence_echantillonnage = 0.1;


//Compteur de millisecondes pour vérifier le système de badge
unsigned long previousMillis=0 ;
unsigned long interval = frequence_echantillonnage*60*10000;

//int frequence_echantillonnage = 1;

//Délai d'attente d'une réponse LoRa (par defaut : 5 secondes)
int delai_reponse_LoRa = 5;

//Taille FIXE Maximale du buffer d'envoi LoRa
uint8_t SIZE_BUFFER = 100;
//Variable a envoyer via LoRa
char buffer_a_envoyer[100];

//pourcentage de la batterie
float SENSOR_BATTERY;



int pinA0=0; 
int pinA1=0; 
int pinA2=0; 


uint32_t compteur=0;

long duree_attente_aleatoire;

//Pour la gestion du bouton
boolean btn_click=false;
//temps pour la lecture du RFID apres avoir appuyé sur le btn
int temps_btn_click=10*1000;
unsigned long compteur_btn_click;

unsigned long compteur_begin_btn_click;
unsigned long compteur_end_btn_click;




void setup()
{
	delay(1000);
	
	Serial.begin(9600);
	Serial.println('Hello!');
  delay(1000);
	
	
	//On vide le buffer_a_envoyer
	memset(buffer_a_envoyer, 0, sizeof(buffer_a_envoyer));
	
	//Alimentation(Vcc) de la LED
	pinMode(13, OUTPUT);
  digitalWrite(13,HIGH);
	
	//Alimentation(Vcc) du 1-Wire
	pinMode(12, OUTPUT);  
  digitalWrite(12,LOW);
	
	//Alimentation(Vcc) du I²C
	pinMode(11, OUTPUT);
  digitalWrite(11,LOW);
	
  pinMode(6, OUTPUT);
  digitalWrite(6,HIGH);
  
	pinMode(A0, INPUT_PULLDOWN);

  //Necessary for Door Open/Close detection
  pinMode(A2, INPUT_PULLUP);
	
	//Alimentation(Vcc) de l'analogique A4
	pinMode(A4, OUTPUT);
  digitalWrite(A4,LOW);
	
	//Alimentation(Vcc) de l'analogique A5
	pinMode(A5, OUTPUT);
  digitalWrite(A5,LOW);
	
	#define VBATPIN A6
		
  Serial1.begin(9600);

  delay(100);
  digitalWrite(13,LOW);
}


long blinkMilli = 0;

void loop()
{
			
	//Variable utilisée pour la récriture du buffer_a_envoyer
	char v1_string[5];
	
	if( millis() - previousMillis >= interval)
	{
		previousMillis = millis(); 
		
		
    digitalWrite(13, HIGH);

		////////////////////////////////////
		//Récupération du niveau de batterie
		////////////////////////////////////
		float measuredvbat = analogRead(VBATPIN);
		measuredvbat *= 2;    // we divided by 2, so multiply back
		measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
		measuredvbat /= 1024; // convert to voltage
		SENSOR_BATTERY=measuredvbat;
		

		
			
		
		//On écrit le header(en-tête) du buffer_a_envoyer + Niveau de batterie
		
		//le "moins" permet d'écrire à gauche
		dtostrf(SENSOR_BATTERY, -5, 3, v1_string);
		
		snprintf(buffer_a_envoyer, SIZE_BUFFER, "%s&B=%s", SENSOR_ID, v1_string);
		
		
		if(Serial)
		{
			
			Serial.println("########################################");
			Serial.println("########## Akuino Sensor v1.0 ##########");
			Serial.println("########################################");
			Serial.print("Fréquence: ");
			Serial.print(frequence_LoRa);
			Serial.print(" MHz");
			Serial.println("");
			Serial.print("Bande passante : ");
			Serial.print(bande_passante_LoRa);
			Serial.print(" KHz");
			Serial.println("");
			Serial.print("Puissance : ");
			Serial.print(puissance_signal_LoRa);
			Serial.print(" mW");
			Serial.println("");
			Serial.print("Niveau de Batterie : " );
			Serial.print(SENSOR_BATTERY);
			Serial.println(" Volt");
			Serial.print("ID : " );
			Serial.println(SENSOR_ID);
			Serial.println("########################################");
			Serial.println("");
			
		}//end if
		
		
		/////////////////////////////////////////////
		//On alimente toutes les sorties du transitor
		
		//Alimentation(Vcc) du 1-Wire
		digitalWrite(12, HIGH);
		
		//Alimentation(Vcc) du I²C
		digitalWrite(11, HIGH);
		
		//Alimentation(Vcc) de l'analogique A4
		digitalWrite(A4, HIGH);
		
		//Alimentation(Vcc) de l'analogique A5
		digitalWrite(A5, HIGH);
		
		//On démarre le 1-Wire
		Wire.begin();
		
		
		////////////////////////////////////////////////
		//Test du port série
		////////////////////////////////////////////////
		if(Serial)
		{
			Serial.println("**********************************************************");
			Serial.println("Test du port série...");
			Serial.println("**********************************************************");
      Serial1.println("TEST PORT SERIE");
      Serial.println("Données envoyées!");
		}
		
		
		////////////////////////////////////////////////
		//Analyse et lecture des analogiques disponibles
		////////////////////////////////////////////////
    pinA0=digitalRead(A0);
    pinA1=analogRead(A1);
    //pinA2=analogRead(A2);
    pinA2=digitalRead(A2);

		if(Serial)
		{
			Serial.println("**********************************************************");
			Serial.println("Analyse et lecture des analogiques disponibles en cours...");
			Serial.println("**********************************************************");
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
		snprintf(buffer_a_envoyer, SIZE_BUFFER, "%s&A0=%s", buffer_a_envoyer, v1_string);
		
        //On écrit
		//le "moins" permet d'écrire à gauche
		dtostrf(pinA1, -1, 0, v1_string);
		
		//On récrit dans la variable du buffer
		snprintf(buffer_a_envoyer, SIZE_BUFFER, "%s&A1=%s", buffer_a_envoyer, v1_string);
		
		//On écrit
		//le "moins" permet d'écrire à gauche
		dtostrf(pinA2, -1, 0, v1_string);
		
		//On récrit dans la variable du buffer
		snprintf(buffer_a_envoyer, SIZE_BUFFER, "%s&A2=%s", buffer_a_envoyer, v1_string);
		
		
		/////////////////////////////////////////
		//Analyse et lecture des I²C disponibles
		////////////////////////////////////////
		
		byte error, address;
		int nDevices;
		float fonction_return_value;
		
		if(Serial)
		{
			Serial.println("***************************************************");
			Serial.println("Analyse et lectures des I²c disponibles en cours...");
			Serial.println("***************************************************");
		}
		
		nDevices = 0;
		//for(address = 1; address < 127; address++ ) 
    address = 0x44;
		{
			/* REMARQUE :
				* Le scanner utilise la valeur de retour de Write.endTransmisstion
				* pour voir si un périphérique a émit un accusé réception de l'adresse
			*/
			Wire.beginTransmission(address);
			error = Wire.endTransmission();
			
			if (error == 0)
			{
				if(Serial)
				{
					Serial.print("Un dispositif(device) I²C a été trouvé à l'adresse 0x");
      					Serial.println(address,HEX);
				}
				
				nDevices++;
				
				
				//////////////////////////////////
				//SHT31 : Température et humidité
				//Adresse I²C: 0x44
				/////////////////////////////////
				
				if(address == 0x44) //Adresse en Hexadécimal
				{
					
					
					if(Serial){Serial.println("Capteur SHT31 (Température+humidité) detecté! Connexion...");}
					if (! sht31.begin(0x44)) {
						Serial.println("Erreur lors de la connexion à ce capteur! :(");
					}
					else
					{
						//on lit les informations de ce capteur
						
						float t = sht31.readTemperature();
						float h = sht31.readHumidity();
						
						if (! isnan(t)) {  // check if 'is not a number'
							Serial.print("Température en °C = "); Serial.println(t);
							} else { 
							Serial.println("Erreur lors de la lecture de la température");
						}
						
						if (! isnan(h)) {  // check if 'is not a number'
							Serial.print("Humidité en % = "); Serial.println(h);
							} else { 
							Serial.println("Erreur lors de la lecture de l'humidité!");
						}
						
						
						//le "moins" permet d'écrire à gauche
						dtostrf(t, -5, 3, v1_string);
						
						//On récrit dans la variable du buffer
						snprintf(buffer_a_envoyer, SIZE_BUFFER, "%s&C1=%s", buffer_a_envoyer, v1_string);
						
						
						//le "moins" permet d'écrire à gauche
						dtostrf(h, -5, 3, v1_string);
						
						//On récrit dans la variable du buffer
						snprintf(buffer_a_envoyer, SIZE_BUFFER, "%s&H1=%s", buffer_a_envoyer, v1_string);
						
						
						
					}
					
				}
			}
			else if (error==4) 
			{
				if(Serial){
				  Serial.print("Erreur inconnue à l'adresse 0x");
					Serial.println(address,HEX);
				}
				
			}    
		}
    if (Serial) {
      if (nDevices == 0) {
        Serial.println("Aucun dispositif(device) n'a été trouvé!");
      } else {
        Serial.println("Analyse du I²C terminée!");
      }
    }
			
		
		///////////////////////////////////////////////////////
		//Analyse et lecture des 1-Wire disponibles en cours...
		///////////////////////////////////////////////////////
		
		if(Serial)
		{
			Serial.println("*****************************************************");
			Serial.println("Analyse et lecture des 1-Wire disponibles en cours...");
			Serial.println("*****************************************************");
		}
		
		byte i;
		byte present = 0;
		byte data[12];
		byte addr[12];
		
		
		
		if ( !ds.search(addr))
		{
			if(Serial){Serial.println("Aucune One-Wire detecté ! :(");}
			/*
				if(Serial){Serial.println("Recherche terminée");}
				
				ds.reset_search();
			*/
		}
		else
		{
			
			if(Serial)
			{
				Serial.print("CHIP ");
				Serial.print(addr[0],HEX);
				//Le premier octet (ROM) indique quelle est le chip (dipositif)
				Serial.print(" = ");
			}
			
			switch (addr[0]) {
				
				case 0x01:
				if(Serial){Serial.println("DS1990 DS2401");}
        //On récrit dans la variable du buffer
        snprintf(buffer_a_envoyer, SIZE_BUFFER, "%s&G=", buffer_a_envoyer);
        for( i = 1; i <= 6; i++) {
          snprintf(buffer_a_envoyer, SIZE_BUFFER, "%s%02x", buffer_a_envoyer, addr[i]);
        }
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
				snprintf(buffer_a_envoyer, SIZE_BUFFER, "%s&C2=%s", buffer_a_envoyer, v1_string);
				
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
				
			} 
			
			
		}
		
		if(Serial)
		{
			Serial.println("----------------------------------------------- FIN DES SCANS -----------------------------------------------");
			Serial.print("BUFFER qu'on doit envoyer au Gateway via LoRa ");
			Serial.print(buffer_a_envoyer);
		}
				
    digitalWrite(13, LOW);
		
		//////////////////////////////////
		//ENVOIE DES INFORMATIONS VIA LoRa
		//////////////////////////////////
		
		if(Serial)
		{
			Serial.println("*****************************************************");
			Serial.println("Transmission des données via LoRa (RF95)");
			Serial.println("*****************************************************");
		}
		
		//Activation du LoRa
		if (!rf95.init())
		{
			if(Serial){Serial.println("Erreur lors de l'initialisation du LoRa (FR95)");}
		}
		else
		{
			if(Serial){Serial.println("Initialisation réussie du LoRa (FR95)!");}
		}
		
		rf95.setFrequency(frequence_LoRa);
		rf95.setTxPower(puissance_signal_LoRa);
		rf95.setSignalBandwidth(bande_passante_LoRa);
		
		rf95.send((uint8_t *)buffer_a_envoyer, 1+strlen(buffer_a_envoyer)); 
		//On attend que le packet ait bien été envoyé...
		rf95.waitPacketSent();
		Serial.println("Réussite: BUFFER(packet) envoyé via LoRa !!!");
		
    digitalWrite(13, HIGH);
		
		Serial.print("On écoute pendant ");
		Serial.print(delai_reponse_LoRa);
		Serial.println(" seconde(s) si on reçoit une éventuelle réponse de notre GateWay...");
		
		// Maintenant, on attend une éventuelle réponse de notre GateWay
		uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
		uint8_t len = sizeof(buf);
		
		if (rf95.waitAvailableTimeout(delai_reponse_LoRa*1000))
		{ 
			// Should be a reply message for us now   
			if (rf95.recv(buf, &len))
			{
				if(Serial)
				{
					Serial.print("Réponse recue : ");
					Serial.println((char*)buf);
					Serial.print("RSSI: ");
					Serial.println(rf95.lastRssi(), DEC); 
				}
				
				
				//On vide le buffer_a_envoyer car il a bien été recu...
				//memset(buffer_a_envoyer, 0, sizeof(buffer_a_envoyer));
				
				//On traite la réponse reçue via notre fonction de traitement de données recues
				//analyse_datas_recues((char*)buf);
				
        digitalWrite(13, LOW);
				//On verifie si le recepteur a bien recu notre réponse...
				if (String((char*)buf) == SENSOR_ID)
				{
					Serial.println("CONFIRMATION OK : Le GATEWAY a bien reçu la réponse!!");
				}
				else
				{
					Serial.println("Interférence avec un autre sensor donc on re envoie le message apres un delai aléatoire!");
					
					//on attend une durée aléatoire avant de renvoyé!
					duree_attente_aleatoire = random(1000,10000);
					
					Serial.print("on attend ");
					Serial.print(duree_attente_aleatoire);
					Serial.println("ms avant de re envoyé...");
					
          delay(duree_attente_aleatoire);
					
					Serial.println("On envoie...");
          digitalWrite(13, HIGH);
					rf95.send((uint8_t *)buffer_a_envoyer, 1+strlen(buffer_a_envoyer)); 
					//On attend que le packet ait bien été envoyé...
					rf95.waitPacketSent();
					Serial.println("Le packet a été re-envoyé");
				}
				
			}
			else
			{
				if(Serial){Serial.println("Problème lors de l'écoute...!! :(");}
			}
		}
		else
		{
			if(Serial){Serial.println("Aucune réponse...");}
		}
		
		if(Serial)
		{
			Serial.println(""); 
			Serial.println("---------------------------------------------");
			Serial.print("Le feather se met en repos(sommeil) pendant ");
			Serial.print(frequence_echantillonnage);
			Serial.print(" min...");
			Serial.println("");
			Serial.println("---------------------------------------------");
		}
		
		//On coupe toute les alimentation
		digitalWrite(13, LOW);
    //Alimentation(Vcc) du 1-Wire
    digitalWrite(12, LOW);
    //Alimentation(Vcc) du I²C
    digitalWrite(11, LOW);
    //Alimentation(Vcc) de l'analogique A4
    digitalWrite(A4, LOW);    
    //Alimentation(Vcc) de l'analogique A5
    digitalWrite(A5, LOW);
	}
	else
	{
		//Le feather est en "veille" mais il écoute toujours qu'on appuie sur le btn du badging

		if(btn_click == true)
		{			
			if(millis() >= compteur_end_btn_click)
			{
        //Alimentation(Vcc) du 1-Wire
        digitalWrite(12, LOW);
				//fin de décompte				
				Serial.println("FIN DE LA LECTURE...");
				btn_click=false;
			}
			else
			{
        //Alimentation(Vcc) du 1-Wire
        digitalWrite(12, HIGH);
        delay(100);
				//on lit la valeur du RFID eventuelle
				
				byte i;
				byte present = 0;
				byte data[12];
				byte addr[8];
				
				
				
				if ( !ds.search(addr))
				{
					//if(Serial){Serial.println("Aucune One-Wire detecté ! :(");}
					/*
						if(Serial){Serial.println("Recherche terminée");}
						
						ds.reset_search();
					*/
				}
				else
				{
					
					switch (addr[0]) {
						
						//Si carte RFID DETECTé
						case 0x01:
            digitalWrite(13, HIGH);
						if(Serial){Serial.println("DS1990 DS2401");}
            //On récrit dans la variable du buffer
            snprintf(buffer_a_envoyer, SIZE_BUFFER, "%s&G=", buffer_a_envoyer);
            for( i = 1; i <= 6; i++) {
              snprintf(buffer_a_envoyer, SIZE_BUFFER, "%s%02x", buffer_a_envoyer, addr[i]);
            }
						compteur_end_btn_click=millis();
						
						Serial.println("ON ENVOIE VIA LoRa COMME QUOI LE BADGE A ETE DETECTER");
						
						Serial.println(buffer_a_envoyer);
						
						//////////////////////////////////
						//ENVOIE DES INFORMATIONS VIA LoRa
						//////////////////////////////////
						
						if(Serial)
						{
							Serial.println("");
							Serial.println(".");
							Serial.println("");
							Serial.println("*****************************************************");
							Serial.println("Transmission des données via LoRa (RF95)");
							Serial.println("*****************************************************");
							Serial.println("");
						}
						
						if(Serial){Serial.println("Initialisation du LoRa RF95...");}
						
						//Activation du LoRa
						if (!rf95.init())
						{
							if(Serial){Serial.println("Erreur lors de l'initialisation du LoRa (FR95) :(");}
						}
						else
						{
							if(Serial){Serial.println("Initialisation réussie du LoRa (FR95)!");}
						}
						
						rf95.setFrequency(frequence_LoRa);
						rf95.setTxPower(puissance_signal_LoRa);
						rf95.setSignalBandwidth(bande_passante_LoRa);
						
						if(Serial){Serial.print("Attente de la disponibilité du cannal de transmission...");}
						if(Serial){Serial.println("(Listen Before Talk ?)");}
						
						
						if(Serial){Serial.println("Le canal libre! On envoie...");}
						
						/*
							uint8_t dataX[50] = "XXXTESTdatasensors";
							rf95.send(dataX, sizeof(dataX));
						*/
						
						
						
						rf95.send((uint8_t *)buffer_a_envoyer, 1+strlen(buffer_a_envoyer)); 
						//On attend que le packet ait bien été envoyé...
						rf95.waitPacketSent();
						Serial.println("Réussite: BUFFER(packet) envoyé via LoRa !!!");
            digitalWrite(13, LOW);
						
						Serial.print("On écoute pendant ");
						Serial.print(delai_reponse_LoRa);
						Serial.println(" seconde(s) si on reçoit une éventuelle réponse de notre GateWay...");
						
						// Maintenant, on attend une éventuelle réponse de notre GateWay
						uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
						uint8_t len = sizeof(buf);
						
						if (rf95.waitAvailableTimeout(delai_reponse_LoRa*1000))
						{ 
							// Should be a reply message for us now   
							if (rf95.recv(buf, &len))
							{
								if(Serial)
								{
									Serial.print("Réponse recue : ");
									Serial.println((char*)buf);
									Serial.print("RSSI: ");
									Serial.println(rf95.lastRssi(), DEC); 
								}
								
								
								//On vide le buffer_a_envoyer car il a bien été recu...
								//memset(buffer_a_envoyer, 0, sizeof(buffer_a_envoyer));
								
								//On traite la réponse reçue via notre fonction de traitement de données recues
								//analyse_datas_recues((char*)buf);
								
								//On verifie si le recepteur a bien recu notre réponse...
								if (String((char*)buf) == SENSOR_ID)
								{
									Serial.println("CONFIRMATION OK : Le GATEWAY a bien reçu la réponse!!");
								}
								else
								{
                  digitalWrite(13, HIGH);
									Serial.println("Interférence avec un autre sensor donc on re envoie le message apres un delai aléatoire!");
									
									//on attend une durée aléatoire avant de renvoyé!
									duree_attente_aleatoire = random(1000,10000);
									
									Serial.print("on attent ");
									Serial.print(duree_attente_aleatoire);
									Serial.println("ms avant de re envoyé...");
									
									delay(duree_attente_aleatoire);
									
									Serial.println("On envoie...");
									rf95.send((uint8_t *)buffer_a_envoyer, 1+strlen(buffer_a_envoyer)); 
									//On attend que le packet ait bien été envoyé...
									rf95.waitPacketSent();
									Serial.println("Le packet a été re-envoyé");
   		            digitalWrite(13, LOW);

									
								}
								
								
								
							}
							else
							{
								if(Serial){Serial.println("Problème lors de l'écoute...!! :(");}
							}
						}
						else
						{
							if(Serial){Serial.println("Aucune réponse...");}
						}
            digitalWrite(13, LOW);
						break;
					} 
					
					
					
					
					
				}
				
				
				
				
			}
			
						
		}
		else
		{
			
			//on lit
			pinA0=digitalRead(A0);
			
			if(pinA0 == 1)
			{
				Serial.println("BTN CLICK, vous avez ");
				Serial.print(temps_btn_click/1000);
				Serial.println(" sec. pour faire passer la carte");
				
				btn_click=true;
				
				compteur_end_btn_click=millis()+temps_btn_click;
				
				digitalWrite(13, HIGH);
				
				//Pour allumer la LED du RFID
				digitalWrite(6, LOW);
				
				
			}
			
		}
		
		
		
	}
	
	
}







////////////////////////////////////////////////////////////////////////
//Fonction pour la lecture de la température via le chip DS18B20 1-Wire
///////////////////////////////////////////////////////////////////////

float lecture_DS18B20(byte addr[8])
{
	byte i;
	byte present = 0;
	byte type_s;
	byte data[12];
	float celsius, fahrenheit;
	
	ds.reset();
	ds.select(addr);
	ds.write(0x44, 1);        // start conversion, with parasite power on at the end
	
	delay(1000);     // maybe 750ms is enough, maybe not
	// we might do a ds.depower() here, but the reset will take care of it.
	
	present = ds.reset();
	ds.select(addr);    
	ds.write(0xBE);         // Read Scratchpad
	
	Serial.print("  Data = ");
	Serial.print(present, HEX);
	Serial.print(" ");
	for ( i = 0; i < 9; i++) {           // we need 9 bytes
		data[i] = ds.read();
		Serial.print(data[i], HEX);
		Serial.print(" ");
	}
	Serial.print(" CRC=");
	Serial.print(OneWire::crc8(data, 8), HEX);
	Serial.println();
	
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
	Serial.print("  Temperature = ");
	Serial.print(celsius);
	Serial.print(" Celsius, ");
	Serial.print(fahrenheit);
	Serial.println(" Fahrenheit");
	
	//on retourne la valeur du capteur
	return(celsius);
	
}//end function


////////////////////////////////////////////////////////////////////////
//Fonction pour l'analyse des données recues par la GateWay
///////////////////////////////////////////////////////////////////////

void analyse_datas_recues(char* datas_recus)
{
	
	//On extrait les 3 premiers caractères
	
	char code_message[3];
	memset(code_message, 0, 4);
	strncpy(code_message, datas_recus, 3);
	puts(code_message);
	
	if(Serial){Serial.println(code_message);}
	
	
	
}
