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

Radio messages are buffered and exchanged in compressed URL format (see project [JBCDIC](https://github.com/AKUINO/JBCDIC/blob/master/README.md), aimed at compressiong JSON, used to compress URLs).

The transmitter is currently based on the Adafruit Feather M0 with a LoRa transmitter (https://www.adafruit.com/product/3178) but next version will be based on Arduino MKR WAN 1300. We add an underlying board to ease connections to sensors and to selectively power them. The following table explains the pinout:

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
J2-10|10|D10 (22)||Switched and limited Vcc (see below)
J2-11|6|D6 (20)||Switched and limited Vcc (see below)
J2-12|v7||D12 (24)|Switched 7 volts to power RFID/NFC reader (small DC/DC converter from D12)

Distributed power is switched by a TBD62783APG DMOS transistor array (clamping diode included), limited by a 300ma temporary fuse, a 18µH inductance and sustained by a 3µF capacitor.

Detailed Eagle schema and board definition are in this directory.

## Data transmitted

Data is encoded as an URL using JBCDIC ( https://github.com/AKUINO/JBCDIC ). A message is made of only one (for now) session of measures up to the LoRa maximum size (53 bytes in our case).

One measurements session is a URL (in the future: a JSON object) with the following properties:

Property|Connection|Pin|Description
--------|----------|---|-----------
H0|||Seconds ellapsed since module startup. H0 is replaced by H with the correct time by the receiver before resending to the Web application
H|||Seconds since 01/01/2019 (if time was broadcasted by receiver ("data sink") and synchronized with transmitter)
N|||Network Identification number: Not transmitted but added by the receiver before resending to the Web application
M|||Radio Module Identification number: Not transmitted but added by the receiver before resending to the Web application
R|||RSSI for this message: Not transmitted but added by the receiver before resending to the Web application
A0|J1-11|A0|Button pressed? Not transmitted if 0
A1|J2-2|A1|Analog input 1. Not transmitted if 0
A2|J2-5|A2|Analog input 2. Not transmitted if 0
B||A6|Battery voltage
B5||A7|USB voltage (if present). Usually 5 volts if connected
B9|||Last RSSI of messages broadcasted by Receiver (decimal)
C|J1-3|RX|Serial data received (string)
Dnnn|J2-8|D5|Value (or array of values or object with different properties) obtained from 1-Wire device at address "nnn..." (decimal)
G|J2-8|D5|Last badge read by RFID reader (encoded in decimal). Not transmitted if none
G0|J1-10|D13|Button LED on? Not transmitted if not
G1|J2-10|D10|Yellow LED of RFID reader is on? Not transmitted if not
G2|J2-11|D6|Red LED of RFID reader is on? Not transmitted if not
Innn|J1-7|I2C|Value (or array of values or object with different properties) obtained from I2C device at address "nnn" (decimal)

### Data transmission example

Before compression to 4 bits per character:
`{H0+3602A0+1B+3-12B9-78D159300978176+27-2I68[+27-5+40-5]}`
which means:
* `H0+3602` : 3602 seconds elapsed since starting the radio module
* `A0+1` : Button is pressed
* `B+3-12` : Battery is 3.12V
* `B9-78` : Last RSSI was -78 (in decimal)
* `D159300978176+27-2` : the temperature sensor address is 0x2517140600 (not including the leading 0x28 indicating its type and the trailing byte providing an CRC) and its temperature is 27.2°C
* `I68[+27-5+40-5]` : the I2C device at address 0x44 (usually an SHT31 sensor) provides two values: 27.5°C and 40.5% rH

The receiver will receive this data and retransmit it to the Internet application in standard JSON and adding network/module identification:

~~~~
  [ { H:(date/time in seconds elapsed since 1/1/1970 as in UNIX),
     N:1,   (network id)
     M:3,   (module id)
     R:-77,   (RSSI of transmitted message)
     A0:1,     
     B:3.12,    
     B9:-78,    
     D159300978176:27.2,    
     I68:[27.5,40.5]   
    } 
  ]
~~~~

## Software
* Radiohead library for encrypted Lora transmissions
* Protohead for simultaneous execution of data collection and radio transmission
* Periodical wake-ups for collection of sensors values and LoRa transmission (Listen before talk, random delay)
* USB Serial port for the configuration wizard
* JBCDIC for data compression
* FIFO Round Robin Buffer for unacknowledged transmitted data
* Watchdog on button for immediate wakeup, NFC/RFID scan and immediate transmission of status data (including the "button pressed" indication)

## Extensions
* To control equipement in adverse electrical environments, insulation of 1-Wire bus may be useful:  http://www.ti.com/lit/ds/symlink/iso1540.pdf
