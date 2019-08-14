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

To have "CE" marked hardware, we will be using M5Stack Core module in the future. It integrates in a sleek package:

    USB Type-C connector (Power and Programming)
    ESP32-based: 4 MByte flash + 520K RAM
    Speaker, 3 Buttons, Color LCD(320*240), 1 Reset
    WiFi 2.4G Antenna: Proant 440
    SD card slot (16G Maximum size)
    Battery Socket & 150 mAh Lipo Battery
    Bus Extension Pins & Holes
    Grove Port

### Pinout

PCB Pin|ESP32 Pin|Remark
-------|---------|------
RST||MCU Reset
3V|Vcc|3V3 power, 250ma for MCU, 250ma available for all other boards
NC||Not connected
GND|GND|GROUND common to all boards
A0|GPIO 26|Relay 1 enable. ADC not working when Wifi works
A1|GPIO 25|Relay 1 disable. ADC not working when Wifi works
A2|GPI 34|Input only!
A3|GPI 39|Input only!
A4|GPI 36|Input only!
A5|GPIO 4|Relay 2 disable. ADC not working when Wifi works
SCK|CLCK|SPI Clock (LoRa)
MO|MOSI|SPI output (LoRa)
MI|MISO|SPI input (LoRa)
RX|RX|Serial1 input
TX|TX|Serial1 output
21|GPIO 21|Relay 2 enable
-------|---------|------
BAT||Battery power
EN||3V3 regulator enable (disable if 3V3 power provided in another way)
USB||USB connector power
13|GPIO 13|Red LED on main board
12|GPIO 12|Pull down resistor
27|GPIO 27|LoRa A=Reset LoRa
33|GPIO 33|LoRa B=ChipSelect SD card
15|GPIO 15|Button A;LoRa C
32|GPIO 32|Button B;LoRa D=InterrupReQuest LoRa
14|GPIO 14|Button C;LoRa E=ChipSelect LoRa
SCL|SCL|I2C Clock (no pull-up)
SDA|SDA|I2C Data (no pull-up). OLED address=0x3C. RTC address=0x68

## Software

The software is a mirror of the one used by the transmitter. It must:
1. acknowledge data received to connecting transmitters (RadioHead Datagrams)
2. TO EVALUATE: assign a reduced numerical id for every string (name, 1-Wire ID, I2C peripheral, etc.) submitted by a transmitter: this allows compression of strings that may need to be transmitted by Radio. If the receiver fails, the transmitters can provide their local part of the dictionary when it restarts.
3. TODO: it retrieves sensors configuration (for the parameters needed to be known locally if any) from the ELSA web application. Those parameters are archived on the SD card in case of Internet failure. Amongst those parameters, alarm levels for some sensors, set points for opening/closing valves or switches, etc.
4. it sends data to the ELSA web application (also archived on the SD card)


