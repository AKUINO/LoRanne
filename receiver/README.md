# Receiver (Data Sink)
One or more gateway (receiver) can collect the data emitted by the different transmitters (radio modules with sensors connected).

The receiver(s) is always listening (never sleeps) and responds immediately when it receives a message:
1. it acknowledges the received message
1. if a further answer is needed (current time for instance), it sends it immediately

The receiver is archiving all the messages it receives. This archive is stored on a SD card which may be removed (the receiver is therefore a data logger).

The receiver sends the received data to the ELSA web application (decompressed and re-expanded to valid JSON but without semantic interpretation). The archived data on the SD card is also useful when Internet is interrupted (even for a very long time): all data can be sent when comunications resume.

The receiver synchronizes its clock with Internet (when available) and store the current date/time in the local RTC.

A small OLED display shows the communication statuses (LoRa, Internet), the latest data received and the possible alarms.

## Hardware

The receiver is a simple stack of the following AdaFruit boards (from bottom to top):
1. SD Card and Real Time Clock: https://www.adafruit.com/product/2922
1. LoRa Radio: https://www.adafruit.com/product/3231
1. HUZZAH32 ESP32 Feather: https://www.adafruit.com/product/3405
1. 128x32 OLED Add-on For Feather: https://www.adafruit.com/product/2900

The enclosure is big enough to guest a 18650 battery for continuous operation even during a long power breakdown.

## Software

The software is a mirror of the one used by the transmitter. It must:
1. answer to connecting transmitters and provide them the date/time (1 second precision, 32 bits number starting 01/01/2019)
2. assign a reduced numerical id for every string (name, 1-Wire ID, I2C peripheral, etc.) submitted by a transmitter: this allows compression of strings that may need to be transmitted by Radio. If the receiver fails, the transmitters can provide their local part of the dictionary when it restarts.
3. it retrieves sensors configuration (for the parameters needed to be known locally if any) from the ELSA web application. Those parameters are archived on the SD card in case of Internet failure. Amongst those parameters, alarm levels for some sensors, set points for opening/closing valves or switches, etc.
4. it sends data to the ELSA web application (also archived on the SD card)


