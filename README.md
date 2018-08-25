# Long Range (LoRa) Radio Sensors for AKUINO
After connecting 17 sensors to a single AKUINO, we were convinced that wireless is very useful...

With the advent of LoRA transmission technology wich ensures a really good indoor range, with the availability of ready to use modules like AdaFruit Feather M0 able to transmit data for years with simple batteries, we are convinced that a very performant solution can be created.
## Hardware
* AdaFruit Feather M0: https://www.adafruit.com/product/3179
* Lithium Battery: https://shop.mchobby.be/accu-et-regulateur/746-accu-lipo-37v-4400mah-3232100007468.html
* Possible case (10 x 10cm, 6cm deep): http://be.farnell.com/fr-BE/fibox/pcm-95-60-g/coffret-boite-polycarbonate-gris/dp/2473443
* 1-Wire sensors have the advantage of being self-identified, "hot-pluggable" and could be connected to one sensor node or another
## Software
* RadioHead libraries: http://www.airspayce.com/mikem/arduino/RadioHead/index.html
  * for LoRa: http://www.airspayce.com/mikem/arduino/RadioHead/classRH__RF95.html
  * For Adafruit Feather M0 with RFM95, construct the driver like this: RH_RF95 rf95(8, 3);
* AdaFruit Feather M0, Arduino IDE SAMD support: https://learn.adafruit.com/adafruit-feather-m0-basic-proto/using-with-arduino-ide
  * UECIDE would be prefered if SAMD suppport / AdaFruit Feather M0 was supported (not really, too bad)
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
  * 7: height bytes floating point number OR 1-Wire address
  * value with the number of bytes given by the format (0 to 8 bytes)
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

## Configuration
We will use a serial terminal (USB) to set the configuration parameters (ANSI character codes for colors). We have the necessary code in C++ for PIC32. The configuration will have to assign a key (0 to 31) to the physically connected ports:
* Digital inputs
* Analog inputs (12 bits)
* Digital outputs (PWM, pull-up, delays) to be SET (messages received)
* 1-Wire (multiple sensors: 1-Wire addresses mapped to different keys)
* Serial link
* Campbell SDI
* I2C device initialization command and register read...
A basic polling cycle (e.g. 3 minutes) has to be set. Each key can be obtained at each poll or at a configured multiple.

## References
* Detailed RadioHead interface: https://github.com/adafruit/RadioHead/blob/master/RH_RF95.h
* The standard pinout of Feather M0 is: https://cdn-learn.adafruit.com/assets/assets/000/046/244/original/adafruit_products_Feather_M0_Basic_Proto_v2.2-1.png?1504885373
* Similar project but for very long range: http://cpham.perso.univ-pau.fr/LORA/WAZIUP/FAQ.pdf
  * http://cpham.perso.univ-pau.fr/LORA/WAZIUP/SUTS-demo-slides.pdf
  

