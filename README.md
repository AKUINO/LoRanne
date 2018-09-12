# Long Range (LoRa) Radio Sensors for AKUINO
After connecting 17 sensors to a single AKUINO, we were convinced that wireless is very useful...

With the advent of LoRA transmission technology which ensures a really good indoor range, with the availability of ready to use modules like AdaFruit Feather M0 able to transmit data for years with simple batteries, we are convinced that a very performant solution can be created.
## Hardware
* AdaFruit Feather M0: https://www.adafruit.com/product/3179
* Lithium Battery: https://shop.mchobby.be/accu-et-regulateur/746-accu-lipo-37v-4400mah-3232100007468.html
* Possible case (10 x 10cm, 6cm deep): http://be.farnell.com/fr-BE/fibox/pcm-95-60-g/coffret-boite-polycarbonate-gris/dp/2473443
* 1-Wire sensors have the advantage of being self-identified, "hot-pluggable" and could be connected to one sensor node or another
## Software
* RadioHead libraries: http://www.airspayce.com/mikem/arduino/RadioHead/index.html
  * for LoRa: http://www.airspayce.com/mikem/arduino/RadioHead/classRH__RF95.html
  * For Adafruit Feather M0 with RFM95, construct the driver like this: RH_RF95 rf95(8, 3);
* 1-Wire Slave device emulation library: https://github.com/orgua/OneWireHub (in case we want to connect to host through 1-wire)
* AdaFruit Feather M0, Arduino IDE SAMD support: https://learn.adafruit.com/adafruit-feather-m0-basic-proto/using-with-arduino-ide
  * UECIDE would be prefered if SAMD suppport / AdaFruit Feather M0 was supported (not really, too bad)
## Sensors
Each node can receive many kind of sensors and its firmware can be adapted for each situation: the USB programming port of the node can be used to either change the firmware or to access a terminal console and change parameters.

An important information is battery voltage: its value will always be read and transmitted (register 1).

Above that, we will start by providing mainly a good support for 1-Wire which which will ensure auto-configuration (no connexion to the USB programming port when some connected devices change).

Each 1-Wire device have a unique 64 bits address which can be auto-discovered by polling the 1-Wire bus (owdir on Linux, the equivalent will have to be programmed using libraries like https://www.pjrc.com/teensy/td_libs_OneWire.html ). The 8 first bits of this address indicates a generic device type which will have to be known by the firmware to adapt itself to the characteristics of each device type (e.g. number of values, polling protocol, parasitic powering...). The 8 last bits are a CRC8 checksum of the 7 previous bytes. It must be verified after each transmission (wire or radio) to ensure data integrity.

List of existing device types: http://owfs.org/index.php?page=family-code-list

The device types that we really need to support at first are:
* 26 for the DS2438 Battery Monitor (used as an ADC for various types of sensors like RH, CO2...)
* 28 for the DS18B20 Temperature Sensor; a library directly supporting this device seems to exist: https://github.com/milesburton/Arduino-Temperature-Control-Library This may be the best way to start supporting some sensors...

A very important information, because we want to support auto-configuration, is the allocation table which maps each 1-Wire device (64 bits ID numbers) to one of the 30 available data registers (see below). In register 0, a checksum of this allocation table must be sent with each radio data packet so the central node knows if it is synchronized with the sensor node (if it has the latest version of the allocation table).

Every minutes or 5 (or any delay set in configuration), the node will transmit the raw data (no scaling) read for the different sensors. The allocation table will be transmitted every hour (or delay set in configuration) or on request by the central node (after detection of a checksum mismatch with the allocation table).

In theory, the receiving side in Linux could simulate an OWFS 1-Wire bus access layer (owserver): application software would see radio data like a standard OWFS locally wired device. In practice, we believe that data will have to be processed by the sensor node to transmit only specific data values like "temperature" and not all registers of the device. A different abstraction is needed and the design of the allocation table will have to take this into account: discussion below...

## Messages
* Encryption: http://www.airspayce.com/mikem/arduino/RadioHead/classRHEncryptedDriver.html#details
* Header with:
  * "FROM" (for sensors, the id of current node; for central node, id to distinguish the right central node)
	  * encoded on one byte
  * "TO" (for sensors, id of their central node; for central node, 255 for broadcast or id of a specific node to be queried)
	  * encoded on one byte
  * "ID", for a given "FROM"-"TO"-"FLAGS" combination, is a sequence ID of the message (overflow at 255): it allows the receiver to detect it has missed messages (and take action to correct this situation if needed).
  * "FLAGS" specifies the type of message: sensors data vs actuators (registers) setting, retransmission request (messages missed by receiver, the receiver signals which ID are missing), devices table to identify connected 1-Wire devices (from sensor node to central)
* Sensors Data message: a 32 bits timestamp followed by a sequence of key-value pairs with our proposed encoding:
  * key: 5 bits (sensor id 0 to 31) (higher bits of 1st byte). A key may appear in multiple key-value pairs (array of values)
  * value encoding format: 3 bits (0 to 7) in the same byte (lower bits)
  * 0: value is zero
  * 1: value is one
  * 2: one byte integer value
  * 3: two bytes signed integer value
  * 4: three bytes signed integer value
  * 5: four bytes signed integer value
  * 6: six bytes small floating point number
  * 7: height bytes floating point number
  * value with the number of bytes given by the format (0 to 8 bytes)
* 1-Wire Allocation table message (sent every hour or on request from the central node):
  * key: 5 bits (sensor id 0 to 31) (higher bits of 1st byte) identifying a data register transmitted in data messages.
  * allocation table field: 3 bits (0 to 7) in the same byte (lower bits)
  * 0 to 6 : to be decided, content will have the same length than the length specified for Data messages values.
  * 7: height bytes = 64bits 1-Wire device address
* Actuators setting messages have the same format but registers (0-31) are independant than those for sensors.
* Retransmission request: timestamp + pairs of bytes for each ID intervals ("begin ID"-"end ID") that needs to be (re)transmitted. Overflow may occur (increment of 255 is 0) and "end ID" can be lower than "begin ID".
* Retransmission: message with exactly the same Header (TO, FROM, ID, FLAGS) and Payload than the one transmitted before

All nodes will be responsible to keep a buffer of the last messages (254 last messages for sensors and 254 last messages for actuators) they sent.

Redundancy of central module is possible: the two modules then share the same ID. A central module may not have received a package and therefore ask to retransmit a message well received by the other module: redundant transmissions are therefore discarded based on ID+timestamp.

## Radio packet representation 
Timestamp is the number of seconds since 01/01/2018 but the lowest values (0, 1, 2, ...) indicates different statuses and conditions needed to be solved before resynchronizing the clocks or resuming transmissions.

	0              5        7
	+-- -- -- -- --+-- -- --+
	|  TIMESTAMP (4 bytes)  |
	+-- -- -- -- --+-- -- --+
	|     FROM (1 byte)     |
	+-- -- -- -- --+-- -- --+
	|      TO (1 byte)      |
	+-- -- -- -- --+-- -- --+
	|   KEY 5 bits | VAL.For|
	|  VALUE (0 To 8 Bytes) |
	+-- -- -- -- --+-- -- --+
	|   KEY 5 bits | VAL.For|
	|  VALUE (0 To 8 Bytes) |
	+-- -- -- -- --+-- -- --+
	|   KEY 5 bits | VAL.For|
	|  VALUE (0 To 8 Bytes) |
	+-- -- -- -- --+-- -- --+

It could contain several sequences of Key, Value Format and Value depending on the number of sensors connected to the module.

## Protocol
* A remote module (sensors) is sleeping most of the time (does not listen to radio) and awakens from time to time (around 3 minutes) to collect data and send it to the central module.
* A central module is listening all the time and sends actuators settings (or retransmission requests) only just after receiving sensors data from a remote module.
* The sender listen to the air and wait for silence (random delay) to avoid collision (this is done by the radio chip SX127x but needs to be configured. Looking at RadioHead library source code, it can be done: https://github.com/adafruit/RadioHead/blob/master/RH_RF95.h ). This is called LBT (Listen Before Talk).
* The message is sent using RadioHead library (encrypted but no ACK required).
* For remote module, the sender keeps listening for eventual input messages from the destination (actuators settings, sensors data retransmission requests) during a short period of time.
* If a remote module notices that some actuators settings are missing, it requests their retransmission.
* Sensor nodes send their 1-Wire Allocation table either periodically (about an hour), either when some connection changes is detected or whenever a request for it is received from the central node

## Configuration
We will use a serial terminal (USB) to set the configuration parameters (ANSI character codes for colors). We have the necessary code in C++ for PIC32. The configuration will have to assign a key (0 to 31) to the physically connected ports:
* Digital inputs: for a 1st version, we will statically map some GPIO as 0/1 ports
* Analog inputs (12 bits): for a 1st version, we will statically map some ADC inputs including at least the battery level and 2 ADC lines
* Digital outputs (PWM, pull-up, delays) to be SET (based on messages received from central): for a 1st version, we will statically map some PWM lines with basic local configuration attributes.
* 1-Wire (multiple sensors: 1-Wire addresses AUTOMATICALLY mapped to different keys but a selection of registries may have to be done based on the device type)
* Serial link (not for a 1st version except if a precise need arise)
* Campbell SDI (not for a 1st version except if a precise need arise)
* I2C device initialization command and register read... (not for a 1st version except if a precise need arise)
A basic polling cycle (e.g. 3 minutes) has to be set. Each key can be obtained either at each poll or at a configured multiple.

## References
* Detailed RadioHead interface: https://github.com/adafruit/RadioHead/blob/master/RH_RF95.h
* The standard pinout of Feather M0 is: https://cdn-learn.adafruit.com/assets/assets/000/046/244/original/adafruit_products_Feather_M0_Basic_Proto_v2.2-1.png?1504885373
* Similar project but for very long range: http://cpham.perso.univ-pau.fr/LORA/WAZIUP/FAQ.pdf
  * http://cpham.perso.univ-pau.fr/LORA/WAZIUP/SUTS-demo-slides.pdf
