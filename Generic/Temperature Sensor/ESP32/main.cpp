#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <WiFi.h>
#include <WebServer.h>

// Constants
#define HOSTNAME "X-TEMPERATURE-1"
#define SSIDNAME ""
#define SSIDPASS ""
#define APIUSER "x-smart"
#define APIPASS "fdTE%G54m2dY!g78"
Adafruit_BMP085 bmp;

// Define web server
WebServer server(80);
String header;


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
  server.on("/api/temperature", handleRoute_temperature);
  server.on("/api/altitude",    handleRoute_altitude);
  server.on("/api/pressure",    handleRoute_pressure);
  server.begin();
  Serial.println("API Server Started.");
  delay(1000);
  
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085/BMP180 sensor, check wiring!");
    while (1) {}
  }
}



void loop() {
  server.handleClient();
}



/**
 * ROUTE - "/"
 */
void handleRoute_root() {
  Serial.println("API Initiated -- Root URL View!");
  
  char msg[2000];
  strcpy(msg, "<html>");
  strcat(msg, "  <head>");
  strcat(msg, "    <title>Welcome to IoT-Home!</title>");
  strcat(msg, "  </head>");
  strcat(msg, "  <body>");
  strcat(msg, "    <main>");
  strcat(msg, "      <section>");
  strcat(msg, "        <h1>Welcome to IoT Home!<h1>");
  strcat(msg, "        <p>");
  strcat(msg, "          Welcome to the IoT Home ESP32-WROOM sensor. Access data using the /api/ urls.");
  strcat(msg, "        </p>");
  strcat(msg, "        <p>");
  strcat(msg, "          <a href='/api/temperature'>/api/temperature</a><br>");
  strcat(msg, "          <a href='/api/pressure'>/api/pressure</a><br>");
  strcat(msg, "          <a href='/api/altitude'>/api/altitude</a><br>");
  strcat(msg, "        </p>");
  strcat(msg, "      </section>");
  strcat(msg, "    </main>");
  strcat(msg, "  </body>");
  strcat(msg, "</html>");
  
  server.send(200, "text/html", msg);
}


/**
 * ROUTE - "/api/temperature"
 */
void handleRoute_temperature() {
  Serial.println("API Initiated -- Temperature Reading!");
  float r = 0.00;
  char r1[10];
  int count = 0;
  char count1[10];
  char msg[1000];
  
  // read sensor
  for (int i=0; i<40; i++) {
    count = i;
    r = bmp.readTemperature();
    if (r > -20 && r < 40)
      break;
    delay(50);
  }

  // format the response
  sprintf (r1, "%f", r);
  sprintf (count1, "%i", count);
  
  strcpy(msg, "{result:{\"ROUTE\":\"temperature\",\"status\":\"OK\", \"value\":\"");
  strcat(msg, r1);
  strcat(msg, "\", ");
  strcat(msg, "\"count\":");
  strcat(msg, count1);
  strcat(msg, "\"}}");

  // Send to client
  server.send(200, "application/json", msg);
}


/**
 * ROUTE - "/api/altitude"
 */
void handleRoute_altitude() {
  Serial.println("API Initiated -- Altitude Reading!");
  float r = 0.00;
  char r1[10];
  int count = 0;
  char count1[2];
  char msg[1000];
  
  // read sensor
  for (int i=0; i<40; i++) {
    count = i;
    r = bmp.readAltitude();
    if (r > -20 && r < 40)
      break;
    delay(50);
  }

  // format the response
  sprintf (r1, "%f", r);
  sprintf (count1, "%i", count);
  
  strcpy(msg, "{result:{\"ROUTE\":\"altitude\",\"status\":\"OK\", \"value\":\"");
  strcat(msg, r1);
  strcat(msg, "\", ");
  strcat(msg, "\"count\":");
  strcat(msg, count1);
  strcat(msg, "\"}}");

  // Send to client
  server.send(200, "application/json", msg);
}


/**
 * ROUTE - "/api/pressure"
 */
void handleRoute_pressure() {
  Serial.println("API Initiated -- Pressure Reading!");
  float r = 0.00;
  char r1[10];
  int count = 0;
  char count1[2];
  char msg[1000];
  
  // read sensor
  for (int i=0; i<40; i++) {
    count = i;
    r = bmp.readPressure();
    if (r > -20 && r < 40)
      break;
    delay(50);
  }

  // format the response
  sprintf (r1, "%f", r);
  sprintf (count1, "%i", count);
  
  strcpy(msg, "{result:{\"ROUTE\":\"pressure\",\"status\":\"OK\", \"value\":\"");
  strcat(msg, r1);
  strcat(msg, "\", ");
  strcat(msg, "\"count\":");
  strcat(msg, count1);
  strcat(msg, "\"}}");

  // Send to client
  server.send(200, "application/json", msg);
}
