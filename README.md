# Radio Sensors for AKUINO
After connecting 17 sensors to a single AKUINO, we were convinced that wireless could be very useful...

The advent of LoRA transmission technology and the availability of ready to use modules like AdaFruit Feather M0 convinced us to develop this improvement.
## Hardware
* AdaFruit Feather M0: https://www.adafruit.com/product/3179
* Lithium Battery: https://shop.mchobby.be/accu-et-regulateur/746-accu-lipo-37v-4400mah-3232100007468.html
* Possible case (10 x 10cm, 6cm deep): http://be.farnell.com/fr-BE/fibox/pcm-95-60-g/coffret-boite-polycarbonate-gris/dp/2473443
## Software
* RadioHead libraries: http://www.airspayce.com/mikem/arduino/RadioHead/index.html
  * for LoRa: http://www.airspayce.com/mikem/arduino/RadioHead/classRH__RF95.html
  * For Adafruit Feather M0 with RFM95, construct the driver like this: RH_RF95 rf95(8, 3);
* AdaFruit Feather M0, Arduino IDE SAMD support: https://learn.adafruit.com/adafruit-feather-m0-basic-proto/using-with-arduino-ide
  * UECIDE would be prefered if SAMD suppport / AdaFruit Feather M0 was supported
## Transmission protocol
* Encryption: http://www.airspayce.com/mikem/arduino/RadioHead/classRHEncryptedDriver.html#details
* Header with:
  * "FROM" (for sensors, id of current node; for central node, id to distinguish the right central node)
	  * encoded on one byte
  * "TO" (for sensors, id of central node; for central node, 255 for broadcast?)
	  * encoded on one byte
* Data message: sequence of key-value pairs with our proposed encoding:
  * key: 5 bits (sensor id 0 to 31) (higher bits of 1st byte). A key may appear in multiple key-value pairs (array of values)
  * value encoding format: 3 bits (0 to 7) in the same byte (lower bits)
  * 0: value is zero
  * 1: value is one
  * 2: one byte integer value
  * 3: two bytes integer value
  * 4: four bytes signed integer value
  * 5: six bytes small floating point number
  * 6: height bytes floating point number
  * 7: reserved!
  * value with the number of bytes given by the format (0 to 8 bytes)

## Radio packet representation 

	0              5        7
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



The standard pinout of Feather M0 is: https://cdn-learn.adafruit.com/assets/assets/000/046/244/original/adafruit_products_Feather_M0_Basic_Proto_v2.2-1.png?1504885373
