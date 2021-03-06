Heat Control Application
====================
By John Chandler, created using numerouse sources

What is it
----------
This program is aimed at building a home heating control system, based on distributed Raspberry Pis opearting
as Sensors, connected back, via WiFi to a Central Control unit.  Again a Raspberry Pi.

A single application is provided that can operate as a Control unit (Master) or a Sensor (Slave).

The program uses wiringPi to control the Pi GPIO for a DHT11 Pressure & Temperature sensor, and
a SSD1331 oled display unit (64 x 96 pixels).


Pin Usage
---------

All pins can be changed in the .h header definitions, however the base setup is as follows:

                     ---------
                     | 1 | 2 | Zone  +5v
                     | 3 | 4 | DHT11 +5v
     Switch Read Pin | 5 | 6 | DHT11 GRND
    Switch Write Pin | 7 | 8 | DHT11 Read/Write Data
         Switch GRND | 9 | 10| HW Read
       Zone 1 Write  | 11| 12| HW Write
       Zone 2 Write  | 13| 14| Zone GRND
       Zone 0 Write  | 15|   |
       O       +3.3v | 17| 18| DC        W
      Br    SDA/MOSI | 19|   |
                     |   | 22| RES/RET  Bl
       R        SCLK | 23| 24| CS/CEO   Gr
       Y        GRND | 25|   |
                     |   |   |

The program uses wiringPi to access the GPIO pins, and utilises Physical pin numbering mode.


