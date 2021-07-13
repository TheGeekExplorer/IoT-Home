# IoT-Home
IoT Suite for a Smart Home consisting of ESP32-WROOM, Arduino Nano/Uno/Mega, and Raspberry Pi


# SENSORS - Temperature Sensor

## Purpose

Each room in a house can have an IoT temperature sensor so that we can monitor centrally the temperature in the house as a whole, and alert the house owner when they've left a window open and it dips below a comfortable temperature.

This alerting can be done on the Raspberry Pi or other IoT central service monitoring the ESP32's JSON API's.

## APIs

#### /api/temperature

Gives you the current temperature in Degrees Centigrade (degC).

#### /api/pressure

Gives you the current air pressure in Pascals (pa).

#### /api/altitude

Gives you the current altitude in Meters (m).

