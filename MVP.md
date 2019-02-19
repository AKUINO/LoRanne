# Capteurs Radio LoRa / Produit viable minimal

Afin de réaliser rapidement un produit qui réponde à 90% des besoins avérés, ce document en liste les caractéristiques ainsi que les étapes de réalisation.

Il est en français car il est à usage de l'équipe aux commandes à Gembloux en ce printemps 2019...

## Spécifications du matériel

1. Module AdaFruit Feather M0 LoRa 868 MHz
1. Batterie LiPo la plus puissante possible dans les limites de taille du boîtier qui sera choisi
1. Boîtier mural pouvant s'ouvrir (vis de serrage) pour faire passer les fils ou capteurs requis tout en gardant une étanchéité IP65 ou plus. S'inspirer de https://cdn-learn.adafruit.com/downloads/pdf/3d-printed-case-for-adafruit-feather.pdf
1. Connexions:
   1. Antenne interne ou externe ? Tester la différence de portée...
   1. Port USB "device" du AdaFruit Feather: pouvoir charger la batterie, pouvoir mettre à jour le logiciel, pouvoir configurer sans ouvrir le boîtier (bouchon en caoutchouc au dessus du port?) 
   1. Alimentation 3V3 commutable (MOSFET? Denis va nous conseiller) pour les périphériques ("high side" ou "low side"?)
   1. Périphériques I2C en chaîne (connecteurs Grove: 4=GND - 3=3V3 - 2=SDA - 1=SCL) de 20cm ou moins.
   1. Port RS-232 (balance, sonde pH, etc.) avec convertisseur de niveaux du genre MAX3232. Idéalement le signal DCE (ou autre?) devrait être lié à un GPIO indiquant si le micro-contrôleur est endormi ou non.
   1. (combien?) Ports GPIO en entrée ou en sortie (bornier à vis), protections sans doute nécessaire (décharges électrostatiques, surtensions, court-circuit...)
   1. (combien?) Ports ADC en entrée (bornier à vis), protections sans doute nécessaire (décharges électrostatiques, surtensions...)
   1. Plusieurs points de connexions devraient être prévus pour la terre (GND) et le VCC (ou le VCC commuté) afin de pouvoir assembler des composants discrets (résistances) pour protéger les entrées.
   1. Bouton "reset" ou ON/OFF pour redémarrer sans devoir ouvrir (panne) ? S'inspirer de https://cdn-learn.adafruit.com/downloads/pdf/on-slash-off-switches.pdf?timestamp=1550577849

## Spécifications du logiciel

1. Voir le README.md ...
1. On préallouera les registres de configuration et de réception des données (pas de nécessité de gérer une allocation dynamique pour le moment): voir plus loin
1. Réseau en étoile (pas de relais "MESH") avec un "data sink" Central (connexion à Internet) pour le moment
1. Bande 10mW (pas de limite d'utilisation)
1. Sortie du Central par le port USB (Série), une ligne par paquet reçu avec encodage "URL": ...?clé=valeur&clé=valeur...
1. Commandes de paramétrage aussi sous forme d'URL: ...?clé=valeur&clé=valeur&...&clé=valeur

## Tableau des Périphériques ("output" du module)

On tient compte du "pinout": https://cdn-learn.adafruit.com/assets/assets/000/046/254/original/feather_Feather_M0_LoRa_v1.2-1.png?1504886587

* TIME, 4 octets: "timestamp" d'envoi: Horloge synchronisée par les messages du Central (data sink)
* BATT, Registre 1, 2 octets: Voltage en mV: Niveau de la batterie
* RSSI, Registre 2, 1 octet: RSSI de réception des messages envoyés par le Central (data sink)
* RX, Registre 3, un ou plusieurs blocs de 1 à 8 octets: données du Port Série (RS232); broches marquées "RX" et "TX"
* I2C: broches marquées "SDA" et "SCL"
  * TEMP, Registre 4, 2 octets: I2C Température en °C x 100
  * RH, Registre 5, 2 octets: I2C Humidité en %rH (% x 100)
  * LUM, Registre 6, 2 octets: I2C Luminosité (0 à 1000 environ)
  * Registre 7 à 11: I2C expansion...
* ADC0 à ADC5, Registre 12 à 17, 2 octets: ADC, broches marquées "A0" à "A5"
* IO09 à IO13, Registre 18 à 23, 0/1: GPIO, broches marquées "9" à "13"
* Broches marquées "5" et "6": contrôle du VCC pour les périphériques, contrôle de flux pour le RS232?
* Autres ?

## Tableau de configuration des Périphériques (paramétrage du module)

On utilisera les mêmes numéros de registres que pour la sortie et les mêmes identifiants dans l'URL d'entrée sauf pour RX qui devient TX...

## Connecter un périphérique RS232 à ELSA

On peut connecter tout périphérique série à une tablette et envoyer des données au clavier de la tablette. Ces données peuvent commencer par un caractère "accent grave" pour que ELSA le reconnaisse comme début d'une opération spéciale (par exemple stocker la donnée transmise). Ceci pourra servir aux balances, aux lecteurs de pH, etc.

On utiliserait un "BlueFruit" avec un sketch implantant un Bluetooth HID: https://learn.adafruit.com/introducing-the-adafruit-bluefruit-spi-breakout/hidkeyboard . On pourrait aussi connecter tout type de capteur I2C ou autre avec tout type de smartphone ou de tablette.
