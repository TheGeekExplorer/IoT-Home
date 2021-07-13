# IoT-Home
IoT Suite for a Smart Home consisting of ESP32-WROOM, Arduino Nano/Uno/Mega, and Raspberry Pi

----

## ROOMS - STAIRS LED LIGHTING

#### Construction and Materials

The LED stairs lighting is implemented using LED strips, MOSFETs with PWM, and ESP32-WROOM units as a WiFi IoT device running bespoke C and C++ code. It also uses a PIR sensor for movement detection at night time.

#### PIR Sensor

The lighting on the stairs can be turned on for a given time (default 60 seconds) when movement is detected on the PIR sensor(s).

#### Web Server over WiFi

The ESP32 is also setup to connect to the local wifi (with credentials) and then host a set of simple REST API to turn on and off the lighting.  There is also scope to make this work with RGB LED lighting for multicoloured solutions, and MOSFETS with PWM for nice glow lighting.

----

## SENSORS - Temperature Sensor

#### Purpose

Each room in a house can have an IoT temperature sensor so that we can monitor centrally the temperature in the house as a whole, and alert the house owner when they've left a window open and it dips below a comfortable temperature.

This alerting can be done on the Raspberry Pi or other IoT central service monitoring the ESP32's JSON API's.

#### APIs

#### /api/temperature

Gives you the current temperature in Degrees Centigrade (degC).

#### /api/pressure

Gives you the current air pressure in Pascals (pa).

#### /api/altitude

Gives you the current altitude in Meters (m).

