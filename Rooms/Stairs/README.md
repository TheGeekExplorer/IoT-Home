# Overview

## Construction and Materials

The LED stairs lighting is implemented using LED strips, MOSFETs with PWM, and ESP32-WROOM units as a WiFi IoT device running bespoke C and C++ code. It also uses a PIR sensor for movement detection at night time.

#### PIR Sensor

The lighting on the stairs can be turned on for a given time (default 60 seconds) when movement is detected on the PIR sensor(s).

#### Web Server over WiFi

The ESP32 is also setup to connect to the local wifi (with credentials) and then host a set of simple REST API to turn on and off the lighting.  There is also scope to make this work with RGB LED lighting for multicoloured solutions, and MOSFETS with PWM for nice glow lighting.

