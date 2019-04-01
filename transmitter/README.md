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
J1-2|T|TX (15)|A3 (8)|Serial TX, 10k pull-up
J1-3|R|RX (14)||Serial RX
J1-4|v||A3 (8)|Switched and limited Vcc (see below)
J1-5|g|||GND
J1-6|C|SCL (18)|D11 (23)|I2C clock, 5k pull-up
J1-7|D|SDA (17)|D11 (23)|I2C data, 5k pull-up
J1-8|v||D11 (23)|Switched and limited Vcc (see below)
J1-9|g|||GND
J1-10|O|D13 (25)||LED: Switched Vcc but limited (see below) and 100 ohm
J1-11|I|A0 (5)||Button switch; Analog input protected and filtered for 50Hz
J1-12|v|||Unswitched Vcc limited by 100 ohm
J2-1|g|||GND
J2-2|1|A1 (6)||Analog input protected and filtered for 50Hz
J2-3|v||A4 (9)|Switched and limited Vcc (see below)
J2-4|g|||GND
J2-5|2|A2 (7)||Analog input protected and filtered for 50Hz
J2-6|v||A5 (10)|Switched and limited Vcc (see below)
J2-7|g|||GND
J2-8|W|D5 (19)|D12 (24)|1-Wire bus, 5k pull-up
J2-9|v||D12 (24)|Switched and limited Vcc (see below)
J2-10|10|D10 (22)|Switched and limited Vcc (see below)
J2-11|6|D6 (20)|Switched and limited Vcc (see below)
J2-12|v7||D12 (24)|Switched 7 volts to power RFID/NFC reader (small DC/DC converter from D12)

Distributed power is switched by a TBD62783APG DMOS transistor array (clamping diode included), limited by a 300ma temporary fuse, a 18µH inductance and sustained by a 3µF capacitor.

Detailed Eagle schema and board definition are in this directory.

## Software
* Radiohead for encrypted Lora transmissions
* Periodical wake-ups for collection of sensors values and LoRa transmission (Listen before talk, random delay)
* USB Serial port for the configuration wizard
* JBCDIC for data compression
* Round Robin Buffer for unacknowledged transmitted data
* Watchdog on button for immediate wakeup, NFC/RFID scan and immediate transmission of status data (including the "button pressed" indication)
