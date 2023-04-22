#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <vector>

// Constants
#define HOSTNAME "X-STAIRS-LED"
#define SSIDNAME ""
#define SSIDPASS ""
#define APIUSER "x-smart"
#define APIPASS "fdTE%G54m2dY!g78"

// Lights Settings
String LightsStatus      = "OFF";
int    LightsTimerLength = 60000;

// GPIO Configuration
int PIR = 15;
std::vector<int> LEDS = {19, 18, 5, 17, 16, 4};

// Define web server
WebServer server(80);
String header;



/**
 * Main Constructor Method
 * @return void
 */
void setup() {

  // Setup SERIAL
  Serial.begin(9600);
  Serial.println("Serial Started.");
  delay(1000);
  
  // Setup WIFI
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.setHostname(HOSTNAME);
  WiFi.setHostname(HOSTNAME);
  WiFi.setHostname(HOSTNAME);
  WiFi.begin(SSIDNAME, SSIDPASS);
  WiFi.setHostname(HOSTNAME);
  WiFi.setHostname(HOSTNAME);
  WiFi.setHostname(HOSTNAME);
  
  // Wait until connected
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(100);
  }

  // Connected
  Serial.println("Connected.");
  Serial.print("Device IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC Address:" );
  Serial.println(WiFi.macAddress());
  Serial.print("SSID:" );
  Serial.println(WiFi.SSID());
  Serial.print("RSSI:" );
  Serial.println(WiFi.RSSI());
  delay(1000);
  
  // Setup web server
  server.on("/", handleRoute_root);
  server.on("/api/turn-on", handleRoute_turnOn);
  server.on("/api/turn-on-timer", handleRoute_turnOnTimer);
  server.on("/api/turn-off", handleRoute_turnOff);
  server.begin();
  Serial.println("API Server Started.");
  delay(1000);
  
  // Setup GPIO -- PIR SENSOR
  Serial.print("Setting up GPIO pins...");
  pinMode(PIR, INPUT);                      // PIR SENSOR
  Serial.print("PIR Sensor DONE...");

  // Setup GPIO -- LED STRIPS
  for (int i=1; i<LEDS.size(); i++) {       // LED STRIPS
    pinMode(LEDS[i], OUTPUT);
    Serial.print("#"); Serial.print(i); Serial.print(" DONE...");
  }

  // Setup GIO -- COMPLETE!
  Serial.println("COMPLETE!");
  
  // Turn all lights off
  lightsOff();
}


/**
 * Main Method Loop
 * @return void
 */
void loop() {

  // Detect for PIR Movement
  int r = digitalRead(PIR);
  if (LightsStatus != "ON" && r > 0) {
    Serial.println("MOTION DETECTED! from PIR Sensor #1");
    lightsOn();
    delay(LightsTimerLength);
    lightsOff();
  }
  
  // Handle client
  server.handleClient();
}



/**
 * ROUTE - "/"
 */
void handleRoute_root() {
  Serial.println("Route Requested: /");
  server.send(200, "application/json", "{result:{\"ROUTE\":\"home\",\"status\":\"OK\",\"Message\":\"Welcome to Smart Lighting!\"}}");
}

/**
 * ROUTE - "/api/turn-on"
 */
void handleRoute_turnOn() {
  Serial.println("API Initiated -- Lights ON!");
  lightsOn();
  server.send(200, "application/json", "{result:{\"ROUTE\":\"turn-on\",\"status\":\"OK\"}}");
}

/**
 * ROUTE - "/api/turn-off"
 */
void handleRoute_turnOff() {
  Serial.println("API Initiated -- Lights OFF!");
  lightsOff();
  server.send(200, "application/json", "{result:{\"ROUTE\":\"turn-off\",\"status\":\"OK\"}}");
}

/**
 * ROUTE - "/api/turn-on-timer"
 */
void handleRoute_turnOnTimer() {
  Serial.println("API Initiated -- Lights ON (TIMER)!");
  lightsOn();
  delay(LightsTimerLength);
  lightsOff();
  server.send(200, "application/json", "{result:{\"ROUTE\":\"turn-on\",\"status\":\"OK\"}}");
}



/**
 * TURN ON LIGHTS - TIMER
 */
void lightsOn () {
    LightsStatus = "ON";
    
    // Run through lights turning them on
    for (int i=0; i<LEDS.size(); i++) {
        digitalWrite(LEDS[i], LOW);
        delay(150);
    }
}
      
      
/**
 * Turn OFF Lights
 */
void lightsOff () {
    LightsStatus = "OFF";
    
    // Turn off lights
    for (int i=0; i<LEDS.size(); i++) {
        digitalWrite(LEDS[i], HIGH);
        //delay(10);
    }
}
