# TRANSMITTER
In a "star" network there are modules collecting data in different places (transmitters) and one or two "data sinks" (receivers, gateways) that route the data to the data management application.

The transmitters must be as autonomous as possible (minimal configuration, automatic adaptation to new sensors put in place, agnostic about the data: it will be managed by the application, not the transmitter).

They include:
* power management: sensors are powered only when needed, battery power is saved as much as possible
* radio bandwidth management: radio transmission is minimized and assumed "dangerous": if it fails, data is kept up to the moment transmission can resume.

The configuration needed is minimal:
* a Network ID is provided once for all and needs a special boot-up sequence and password to be changed.
* a Network access key and a unique Module ID must be set. They remain normally unchanged. Another password protects those parameters.

The peripherals are automatically detected (1-Wire, I2C). Some may have to be configured (Analog or serial inputs). This is done through a wizard (sequence of questions with current answers provided by defaults).

Radio messages are buffered and exchanged in compressed JSON format (see project [JBCDIC](https://github.com/AKUINO/JBCDIC/blob/master/README.md)).

The transmitter is based on the Adafruit Feather M0 with a LoRa transmitter (https://www.adafruit.com/product/3178). We add an underlying board to ease connections to sensors and to selectively power them. The following table explains the pinout:

Connector|Code|Data Pin|Power Activation Pin|Remark
---------|----|--------|--------------------|------
J1-1|g|||GND
J1-2|T|||Serial TX|GPIOA3 (8)
J1-3|R|||Serial RX|GPIOA3 (8)
J1-4|v||A3 (8)
J1-5|g|||GND
J1-6|C|SCL|GPIO11 (23)
J1-7|D|SDA|GPIO11 (23)
J1-8|v||GPIO11 (23)
J1-9|g|||GND
J1-10|O|GPIO13 (25)|limited by 100 ohm (LED)
J1-11|I|A0 (5)|Button switch; Protected and filtered for 50Hz
J1-12|v||VCC limited by 100 ohm
J2-1|g|||GND
J2-2|1|A1 (6)|Protected and filtered for 50Hz
J2-3|v||A4 (9)
J2-4|g|||GND
J2-5|2|A2 (7)|Protected and filtered for 50Hz
J2-6|v||A5 (10)
J2-7|g|||GND
J2-8|1|GPIO5|GPIO12 (24)
J2-9|v||GPIO12 (24)
J2-10|10|GPIO10 (22)
J2-11|6|GPIO6 (20)
J2-12|v7||GPIO12 (24)|7 volts to power RFID/NFC reader

Distributed power is protected by a 300ma temporary fuse and by a 18µH inductance and a 3µF capacitor.
