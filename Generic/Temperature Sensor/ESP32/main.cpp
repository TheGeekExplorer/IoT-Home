#include <stdlib.h>
#include <Wire.h>
#include <time.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include "mbedtls/md.h"
#include "DHT.h"
#include <Adafruit_BMP085.h>

#define DHTPIN1 3
#define DHTPIN2 23
#define DHTTYPE DHT11   // DHT 11

// API Config
char APIUSER[20]  = "x-smart";
char APIPASS[20]  = "password";

// WiFi Config Variables
char SSIDNAME[30];
char SSIDPASS[30];
char HOSTNAME[30];
char MODE[1];

// EEPROM Config
#define EEPROM_SIZE 255
int  EEPROM_SSIDNAME_LOCATION       = 1;
int  EEPROM_SSIDPASS_LOCATION       = 36;
int  EEPROM_HOSTNAME_LOCATION       = 71;
int  EEPROM_MODE_LOCATION           = 120;
int  EEPROM_SSIDNAME_ISSET_LOCATION = 111;
int  EEPROM_SSIDPASS_ISSET_LOCATION = 113;
int  EEPROM_HOSTNAME_ISSET_LOCATION = 115;
int  EEPROM_MODE_ISSET_LOCATION     = 116;

// Other Variables
String SESSION_COOKIE_KEY = "none";
String header;
int relayOnePin = 26;

// Instantiate Objects / Devices
WebServer server(80);
Adafruit_BMP085 bmp;
DHT dhtA(DHTPIN1, DHTTYPE);
DHT dhtB(DHTPIN2, DHTTYPE);


/**
   Setup method for ESP32
   @param void
   @return void
*/
void setup() {

  // SERIAL - Setup serial
  Serial.begin(9600);
  Serial.println("> Serial Started.");
  delay(1000);

  // EEPROM - Initialise
  EEPROM.begin(EEPROM_SIZE);
  bool isCreds    = getWiFiCedentials();  // Gets SSID / SSID PASS from EEPROM and stores in SSIDNAME / SSIDPASS globals.
  bool isHostname = getHostname();        // Gets HOSTNAME from EEPROM and stores in HOSTNAME global.
  delay(1000);

  
  // If no creds stored in EEPROM then SETUP via Blutooth, else
  // connect to WiFi
  if (!isCreds) {
    
    Serial.println("> Running blutooth device setup program...");

    // Wipe the EEPROM
    wipeEEPROM(); delay(2000);
    
    // Start an AP so you can connect to it to setup the device
    char apSSIDNAME[30];
    char apSSIDPASS[30];
    strcat(apSSIDNAME, "IoT Smart Home");
    strcat(apSSIDPASS, "password");
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSIDNAME, apSSIDPASS);
    Serial.print("> SSID NAME: '"); Serial.print(apSSIDNAME); Serial.println("'");
    Serial.print("> SSID PASS: '"); Serial.print(apSSIDPASS); Serial.println("'");
    delay(3000);
  
  // Else connect to WIFI and run!
  } else {
    
    Serial.println("> Booting...");
    
    // WIFI - Setup wifi
    Serial.print("> Connecting to WiFi");
    Serial.print("> SSID NAME: '"); Serial.print(SSIDNAME); Serial.println("'");
    Serial.print("> SSID PASS: '"); Serial.print(SSIDPASS); Serial.println("'");
    Serial.print("> HOSTNAME:  '"); Serial.print(HOSTNAME); Serial.println("'");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(); delay(100);
    WiFi.begin(SSIDNAME, SSIDPASS); WiFi.setSleep(false);

    // If no HOSTNAME configured then use default
    if (!isHostname)
      strcat(HOSTNAME, "X-SENSOR");
    WiFi.setHostname(HOSTNAME);
  
    // WIFI - Wait until connected
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print('.');
      delay(100);
    }

    // WIFI - Display connection details
    Serial.println("Connected.");


  }
  
  Serial.print(">> Device IP: ");   Serial.println(WiFi.localIP());
  Serial.print(">> MAC Address:" ); Serial.println(WiFi.macAddress());
  Serial.print(">> SSID:" );        Serial.println(WiFi.SSID());
  Serial.print(">> RSSI:" );        Serial.println(WiFi.RSSI());
  delay(1000);
  
  // WEB SERVER - Define Routes
  server.on("/",                           HTTP_GET,  handleRoute_root);
  server.on("/authentication",             HTTP_POST, handleRoute_authentication);
  server.on("/authentication/logout",      HTTP_GET,  handleRoute_authentication_logout);
  server.on("/dashboard",                  HTTP_GET,  handleRoute_dashboard);
  server.on("/system/wipe-eeprom",         HTTP_GET,  handleRoute_system_wipe_eeprom);
  server.on("/system/set-mode",            HTTP_GET,  handleRoute_newMode);
  server.on("/system/set-mode/save",       HTTP_POST, handleRoute_newMode_Save);
  server.on("/system/set-wifi",            HTTP_GET,  handleRoute_newWifiDetails);
  server.on("/system/set-wifi/save",       HTTP_POST, handleRoute_newWifiDetails_Save);
  server.on("/system/set-hostname",        HTTP_GET,  handleRoute_newHostname);
  server.on("/system/set-hostname/save",   HTTP_POST, handleRoute_newHostname_Save);
  server.on("/api/identity.json",          HTTP_GET,  handleRoute_identity);
  server.on("/api/temperature.json",       HTTP_GET,  handleRoute_temperature);
  server.on("/weather/temperature",        HTTP_GET,  handleRoute_temperatureHTML);
  server.on("/api/pressure.json",          HTTP_GET,  handleRoute_pressure);
  server.on("/weather/pressure",           HTTP_GET,  handleRoute_pressureHTML);
  server.on("/api/humidity.json",          HTTP_GET,  handleRoute_humidity);
  server.on("/weather/humidity",           HTTP_GET,  handleRoute_humidityHTML);
  
  // WEB SERVER - Define which request headers you need access to
  const char *headers[] = {"Host", "Referer", "Cookie"};
  size_t headersCount = sizeof(headers) / sizeof(char*);
  server.collectHeaders(headers, headersCount);
  
  // WEB SERVER - Begin!
  server.begin();
  Serial.println("> API Server Started.");
  delay(1000);
  
  // BMP180
  Wire.begin(17,18); //new SDA SCL pins (D6 and D4 for esp8266)
  delay(100);
  
  if (!bmp.begin(0x76, &Wire)) {
    Serial.println("> ERROR - Could not find a valid BMP085/BMP180 sensor, check wiring!");
    Serial.println(">> Hanging.");
  //  while (1) {}
  }

  // DHT11
  dhtA.begin();
  dhtB.begin();
  
  // Completed boot
  Serial.println("> Boot Completed. Waiting for HTTP Requests.");
}


/**
   Main loop method
   @param void
   @return void
*/
void loop() {
  server.handleClient();
}


/**
   ROUTE - "/"
   @param void
   @return void
*/
void handleRoute_root() {
  char msg[2000];
  strcpy(msg, "<html>");
  strcat(msg, "  <head>");
  strcat(msg, "    <title>");  strcat(msg, HOSTNAME);  strcat(msg, " - ESP32 SENSOR</title>\r\n");
  strcat(msg, "    <style type=\"text/css\">\r\n");
  strcat(msg, "      body  { background-color:#555; font-size:16px; color:#444; font-family: Sans-Serif; box-sizing: border-box; }\r\n");
  strcat(msg, "      main  { margin:5% auto; width:100%; max-width:360px; padding:30px 40px; background-color:#eee; border-radius:10px; box-shadow:0 0 20px #222; }\r\n");
  strcat(msg, "      h1    { font-size:24px; font-weight:bold; color:#D52E84; }\r\n");
  strcat(msg, "      p     { font-size:16px; font-weight:normal; }\r\n");
  strcat(msg, "      a     { color:#D52E84; } a:visited { color:#D52E84; }\r\n");
  strcat(msg, "      input { border:2px solid #bbb; color:#444; border-radius:5px; padding:5px; }\r\n");
  strcat(msg, "    </style>\r\n");
  strcat(msg, "  </head>");
  strcat(msg, "  <body>");
  strcat(msg, "    <main>");
  strcat(msg, "<h1>");  strcat(msg, HOSTNAME);  strcat(msg, " - LOGIN</h1>\r\n");
  strcat(msg, "<p>");
  strcat(msg, "  <form action=\"/authentication\" method=\"POST\">");
  strcat(msg, "    <h3>Username</h3>");
  strcat(msg, "    <input type=\"text\" name=\"username\"><br>");
  strcat(msg, "    <h3>Password</h3>");
  strcat(msg, "    <input type=\"password\" name=\"password\"><br><br>");
  strcat(msg, "    <input type=\"submit\" value=\"Login!\"><br>");
  strcat(msg, "  </form>");
  strcat(msg, "</p>");
  strcat(msg, "    </main>");
  strcat(msg, "  </body>");
  strcat(msg, "</html>");

  // Send content to client
  setHeaders_NoCache();
  setHeaders_CrossOrigin();
  server.send(200, "text/html", msg);
}


/**
   ROUTE - "/dashboard/"
   @param void
   @return void
*/
void handleRoute_dashboard() {
  if (checkCookieAuthed()) {
  
    // Else, build content for page
    char msg[2000];
    strcpy(msg, "<html>");
    strcat(msg, "  <head>");
    strcat(msg, "    <title>");  strcat(msg, HOSTNAME);  strcat(msg, " - ESP32 SENSOR</title>\r\n");
    strcat(msg, "    <style type=\"text/css\">\r\n");
    strcat(msg, "      body  { background-color:#555; font-size:16px; color:#444; font-family: Sans-Serif; box-sizing: border-box; }\r\n");
    strcat(msg, "      main  { margin:5% auto; width:100%; max-width:360px; padding:30px 40px; background-color:#eee; border-radius:10px; box-shadow:0 0 20px #222; }\r\n");
    strcat(msg, "      h1    { font-size:24px; font-weight:bold; color:#D52E84; }\r\n");
    strcat(msg, "      p     { font-size:16px; font-weight:normal; }\r\n");
    strcat(msg, "      a     { color:#D52E84; } a:visited { color:#D52E84; }\r\n");
    strcat(msg, "      input { border:2px solid #bbb; color:#444; border-radius:5px; padding:5px; }\r\n");
    strcat(msg, "    </style>\r\n");
    strcat(msg, "  </head>");
    strcat(msg, "  <body>");
    strcat(msg, "    <main>");
    strcat(msg, "  <h1>DASHBOARD<h1>");
    strcat(msg, "  <h3>Webapp Pages<h3>");
    strcat(msg, "  <p>");
    strcat(msg, "    <!-- <a href='/leds/relay-on'>LEDS ON</a> / <a href='/leds/relay-off'>OFF</a><br> -->");
    strcat(msg, "    <a href='/weather/light'>Light</a><br>");
    strcat(msg, "    <a href='/weather/temperature'>Temperature</a><br>");
    strcat(msg, "    <a href='/weather/pressure'>Pressure</a><br>");
    strcat(msg, "    <a href='/weather/humidity'>Humidity</a><br>");
    strcat(msg, "  </p>");
    strcat(msg, "  <h3>API Endpoints<h3>");
    strcat(msg, "  <p>");
    strcat(msg, "    <a href='/api/light.json'>/api/light.json</a><br>");
    strcat(msg, "    <a href='/api/temperature.json'>/api/temperature.json</a><br>");
    strcat(msg, "    <a href='/api/pressure.json'>/api/pressure.json</a><br>");
    strcat(msg, "    <a href='/api/humidity.json'>/api/humidity.json</a><br>");
    strcat(msg, "  </p>");
    strcat(msg, "  <h3>System<h3>");
    strcat(msg, "  <p>");
    strcat(msg, "    <a href='/system/set-mode'>Set Device Mode</a><br>");
    strcat(msg, "    <a href='/system/set-wifi'>Set new Wifi Details</a><br>");
    strcat(msg, "    <a href='/system/set-hostname'>Set new Hostname</a><br>");
    strcat(msg, "    <a href='/system/wipe-eeprom'>Wipe EEPROM</a><br>");
    strcat(msg, "  </p>");
    strcat(msg, "  <p>");
    strcat(msg, "    <a href='/authentication/logout'>[x] Logout</a><br>");
    strcat(msg, "  </p>");
    strcat(msg, "    </main>");
    strcat(msg, "  </body>");
    strcat(msg, "</html>");
  
    // Send content to client
    setHeaders_NoCache();
    setHeaders_CrossOrigin();
    server.send(200, "text/html", msg);
  }
}


/**
   ROUTE - "/api/temperature1.json"
   @param void
   @return void
*/
void handleRoute_temperature() {
  if (checkCookieAuthed()) {
  
    // Define variables
    float r1 = 0.00;
    float r2 = 0.00;
    float r3 = 0.00;
    float r4 = 0.00;
    char rr1[10], rr2[10], rr3[10], rr4[10];
    char msg[255];
    
    // Read sensor
    r1 = readSensor("temperature1");  // Read sensor
    r1 = roundf(r1 * 100) / 100;      // Round to 2 decimal places
    r2 = readSensor("temperature2");  // Read sensor
    r2 = roundf(r2 * 100) / 100;      // Round to 2 decimal places
    r3 = readSensor("temperature2");  // Read sensor
    r3 = roundf(r3 * 100) / 100;      // Round to 2 decimal places
    r4 = getAvg(r1, r2, r3);
    r4 = roundf(r4 * 100) / 100;      // Round to 2 decimal places
  
    sprintf (rr1, "%f", r1);          // Convert to Char Array
    sprintf (rr2, "%f", r2);          // Convert to Char Array
    sprintf (rr3, "%f", r3);          // Convert to Char Array
    sprintf (rr4, "%f", r4);          // Convert to Char Array
    
    strcpy(msg, "{result:{\"ROUTE\":\"temperature\",\"status\":\"OK\", ");
    strcat(msg, "\"t1\":\"");  strcat(msg, rr1); strcat(msg, "\",");
    strcat(msg, "\"t2\":\"");  strcat(msg, rr2); strcat(msg, "\",");
    strcat(msg, "\"t3\":\"");  strcat(msg, rr3); strcat(msg, "\",");
    strcat(msg, "\"avg\":\""); strcat(msg, rr4); strcat(msg, "\",");
    strcat(msg, "}}");
  
    // Send content to client
    setHeaders_NoCache();
    setHeaders_CrossOrigin();
    server.send(200, "application/json", msg);
  }
}


/**
   ROUTE - "/weather/temperature1"
   @param void
   @return void
*/
void handleRoute_temperatureHTML() {
  if (checkCookieAuthed()) {
  
    // Define variables
    float r1 = 0.00;
    float r2 = 0.00;
    float r3 = 0.00;
    float r4 = 0.00;
    char rr1[10], rr2[10], rr3[10], rr4[10];
    
    // Read sensor
    r1 = readSensor("temperature1");  // Read sensor
    r1 = roundf(r1 * 100) / 100;      // Round to 2 decimal places
    r2 = readSensor("temperature2");  // Read sensor
    r2 = roundf(r2 * 100) / 100;      // Round to 2 decimal places
    r3 = readSensor("temperature2");  // Read sensor
    r3 = roundf(r3 * 100) / 100;      // Round to 2 decimal places
    r4 = getAvg(r1, r2, r3);
    r4 = roundf(r4 * 100) / 100;      // Round to 2 decimal places
  
    sprintf (rr1, "%f", r1);          // Convert to Char Array
    sprintf (rr2, "%f", r2);          // Convert to Char Array
    sprintf (rr3, "%f", r3);          // Convert to Char Array
    sprintf (rr4, "%f", r4);          // Convert to Char Array
  
    // Else, build content for page
    char msg[2000];
    strcpy(msg, "<html><head><title>");  strcat(msg, HOSTNAME); strcat(msg, " - ESP32 SENSOR</title>\r\n");
    strcat(msg, "  <style type=\"text/css\">\r\n");
    strcat(msg, "    body  { background-color:#555; font-size:16px; color:#444; font-family: Sans-Serif; box-sizing: border-box; }\r\n");
    strcat(msg, "    main  { margin:5% auto; width:100%; max-width:360px; padding:30px 40px; background-color:#eee; border-radius:10px; box-shadow:0 0 20px #222; }\r\n");
    strcat(msg, "    h1    { font-size:24px; font-weight:bold; color:#D52E84; }\r\n");
    strcat(msg, "    p     { font-size:16px; font-weight:normal; }\r\n");
    strcat(msg, "    a     { color:#D52E84; } a:visited { color:#D52E84; }\r\n");
    strcat(msg, "    input { border:2px solid #bbb; color:#444; border-radius:5px; padding:5px; }\r\n");
    strcat(msg, "  </style>\r\n");
    strcat(msg, "  </head><body>");
    strcat(msg, "    <main>");
    strcat(msg, "      <h1>Temperature</h1>\r\n");
    strcat(msg, "      <div style='width:100%; height:200px; clear:both;'>\r\n");
    strcat(msg, "        <div style='width:49%; float:left;'>\r\n");
    strcat(msg, "            <h3>&laquo;ICON&raquo;</h3>\r\n");
    strcat(msg, "        </div>\r\n");
    strcat(msg, "        <div style='width:49%; float:left;'>\r\n");
    strcat(msg, "            <h3>T1:  "); strcat(msg, rr1); strcat(msg, " C</h3>\r\n");
    strcat(msg, "            <h3>T2:  "); strcat(msg, rr2); strcat(msg, " C</h3>\r\n");
    strcat(msg, "            <h3>T3:  "); strcat(msg, rr3); strcat(msg, " C</h3>\r\n");
    strcat(msg, "            <h3>AVG: "); strcat(msg, rr4); strcat(msg, " C</h3>\r\n");
    strcat(msg, "        </div>\r\n");
    strcat(msg, "      </div>\r\n");
    strcat(msg, "      <p>");
    strcat(msg, "        <a href='/dashboard'>[x] Back</a><br>");
    strcat(msg, "      </p>");
    strcat(msg, "    </main>");
    strcat(msg, "  </body>");
    strcat(msg, "</html>");
  
    // Send content to client
    setHeaders_NoCache();
    setHeaders_CrossOrigin();
    server.send(200, "text/html", msg);
  }
}


/**
   ROUTE - "/api/temperature1.json"
   @param void
   @return void
*/
void handleRoute_pressure() {
  if (checkCookieAuthed()) {
  
    // Define variables
    float r1 = 0.00;
    char rr1[10];
    char msg[255];
    
    // Read sensor
    r1 = readSensor("pressure");  // Read sensor
    r1 = roundf(r1 * 100) / 100;      // Round to 2 decimal places
    sprintf (rr1, "%f", r1);          // Convert to Char Array
    
    strcpy(msg, "{result:{\"ROUTE\":\"pressure\",\"status\":\"OK\", ");
    strcat(msg, "\"p1\":\"");  strcat(msg, rr1); strcat(msg, "\",");
    strcat(msg, "}}");
  
    // Send content to client
    setHeaders_NoCache();
    setHeaders_CrossOrigin();
    server.send(200, "application/json", msg);
  }
}


/**
   ROUTE - "/weather/temperature1"
   @param void
   @return void
*/
void handleRoute_pressureHTML() {
  if (checkCookieAuthed()) {
  
    // Define variables
    float r1 = 0.00;
    char rr1[10];
    
    // Read sensor
    r1 = readSensor("pressure");      // Read sensor
    r1 = roundf(r1 * 100) / 100;      // Round to 2 decimal places
    sprintf (rr1, "%f", r1);          // Convert to Char Array
  
    // Else, build content for page
    char msg[2000];
    strcpy(msg, "<html><head><title>");  strcat(msg, HOSTNAME); strcat(msg, " - ESP32 SENSOR</title>\r\n");
    strcat(msg, "  <style type=\"text/css\">\r\n");
    strcat(msg, "    body  { background-color:#555; font-size:16px; color:#444; font-family: Sans-Serif; box-sizing: border-box; }\r\n");
    strcat(msg, "    main  { margin:5% auto; width:100%; max-width:360px; padding:30px 40px; background-color:#eee; border-radius:10px; box-shadow:0 0 20px #222; }\r\n");
    strcat(msg, "    h1    { font-size:24px; font-weight:bold; color:#D52E84; }\r\n");
    strcat(msg, "    p     { font-size:16px; font-weight:normal; }\r\n");
    strcat(msg, "    a     { color:#D52E84; } a:visited { color:#D52E84; }\r\n");
    strcat(msg, "    input { border:2px solid #bbb; color:#444; border-radius:5px; padding:5px; }\r\n");
    strcat(msg, "  </style>\r\n");
    strcat(msg, "  </head><body>");
    strcat(msg, "    <main>");
    strcat(msg, "      <h1>Pressure</h1>\r\n");
    strcat(msg, "      <div style='width:100%; height:200px; clear:both;'>\r\n");
    strcat(msg, "        <div style='width:49%; float:left;'>\r\n");
    strcat(msg, "            <h3>&laquo;ICON&raquo;</h3>\r\n");
    strcat(msg, "        </div>\r\n");
    strcat(msg, "        <div style='width:49%; float:left;'>\r\n");
    strcat(msg, "            <h3>P1: "); strcat(msg, rr1); strcat(msg, " mb</h3>\r\n");
    strcat(msg, "        </div>\r\n");
    strcat(msg, "      </div>\r\n");
    strcat(msg, "      <p>");
    strcat(msg, "        <a href='/dashboard'>[x] Back</a><br>");
    strcat(msg, "      </p>");
    strcat(msg, "    </main>");
    strcat(msg, "  </body>");
    strcat(msg, "</html>");
  
    // Send content to client
    setHeaders_NoCache();
    setHeaders_CrossOrigin();
    server.send(200, "text/html", msg);
  }
}


/**
   ROUTE - "/api/humidity.json"
   @param void
   @return void
*/
void handleRoute_humidity() {
  if (checkCookieAuthed()) {
  
    // Define variables
    float r1 = 0.00;
    float r2 = 0.00;
    char rr1[10], rr2[10];
    char msg[255];
    
    // Read sensor
    r1 = readSensor("humidity1");     // Read sensor
    r1 = roundf(r1 * 100) / 100;      // Round to 2 decimal places
    r2 = readSensor("humidity2");     // Read sensor
    r2 = roundf(r2 * 100) / 100;      // Round to 2 decimal places
  
    sprintf (rr1, "%f", r1);          // Convert to Char Array
    sprintf (rr2, "%f", r2);          // Convert to Char Array
    
    strcpy(msg, "{result:{\"ROUTE\":\"humidity\",\"status\":\"OK\", ");
    strcat(msg, "\"h1\":\"");  strcat(msg, rr1); strcat(msg, "\",");
    strcat(msg, "\"h2\":\"");  strcat(msg, rr2); strcat(msg, "\",");
    strcat(msg, "}}");
  
    // Send content to client
    setHeaders_NoCache();
    setHeaders_CrossOrigin();
    server.send(200, "application/json", msg);
  }
}


/**
   ROUTE - "/weather/humidity"
   @param void
   @return void
*/
void handleRoute_humidityHTML() {
  if (checkCookieAuthed()) {
  
    // Define variables
    float r1 = 0.00;
    float r2 = 0.00;
    char rr1[10], rr2[10];
    
    // Read sensor
    r1 = readSensor("humidity1");  // Read sensor
    r1 = roundf(r1 * 100) / 100;      // Round to 2 decimal places
    r2 = readSensor("humidity2");  // Read sensor
    r2 = roundf(r2 * 100) / 100;      // Round to 2 decimal places
  
    sprintf (rr1, "%f", r1);          // Convert to Char Array
    sprintf (rr2, "%f", r2);          // Convert to Char Array
  
    // Else, build content for page
    char msg[2000];
    strcpy(msg, "<html><head><title>");  strcat(msg, HOSTNAME); strcat(msg, " - ESP32 SENSOR</title>\r\n");
    strcat(msg, "  <style type=\"text/css\">\r\n");
    strcat(msg, "    body  { background-color:#555; font-size:16px; color:#444; font-family: Sans-Serif; box-sizing: border-box; }\r\n");
    strcat(msg, "    main  { margin:5% auto; width:100%; max-width:360px; padding:30px 40px; background-color:#eee; border-radius:10px; box-shadow:0 0 20px #222; }\r\n");
    strcat(msg, "    h1    { font-size:24px; font-weight:bold; color:#D52E84; }\r\n");
    strcat(msg, "    p     { font-size:16px; font-weight:normal; }\r\n");
    strcat(msg, "    a     { color:#D52E84; } a:visited { color:#D52E84; }\r\n");
    strcat(msg, "    input { border:2px solid #bbb; color:#444; border-radius:5px; padding:5px; }\r\n");
    strcat(msg, "  </style>\r\n");
    strcat(msg, "  </head><body>");
    strcat(msg, "    <main>");
    strcat(msg, "      <h1>Humidity</h1>\r\n");
    strcat(msg, "      <div style='width:100%; height:200px; clear:both;'>\r\n");
    strcat(msg, "        <div style='width:49%; float:left;'>\r\n");
    strcat(msg, "            <h3>&laquo;ICON&raquo;</h3>\r\n");
    strcat(msg, "        </div>\r\n");
    strcat(msg, "        <div style='width:49%; float:left;'>\r\n");
    strcat(msg, "            <h3>H1:  "); strcat(msg, rr1); strcat(msg, " C</h3>\r\n");
    strcat(msg, "            <h3>H2:  "); strcat(msg, rr2); strcat(msg, " C</h3>\r\n");
    strcat(msg, "        </div>\r\n");
    strcat(msg, "      </div>\r\n");
    strcat(msg, "      <p>");
    strcat(msg, "        <a href='/dashboard'>[x] Back</a><br>");
    strcat(msg, "      </p>");
    strcat(msg, "    </main>");
    strcat(msg, "  </body>");
    strcat(msg, "</html>");
  
    // Send content to client
    setHeaders_NoCache();
    setHeaders_CrossOrigin();
    server.send(200, "text/html", msg);
  }
}


/**
   ROUTE - "/api/identity.json"
   Exists so that it can be identifed on the network because
   we cannot rely on the IP address as it changes.
   @param void
   @return void
*/
void handleRoute_identity() {
  setHeaders_NoCache();
  setHeaders_CrossOrigin();

  // Define identity
  char msg[255];

  // Build content
  strcpy(msg, "{result:{\"ROUTE\":\"identity\",\"status\":\"OK\", \"value\":\"");
  strcat(msg, HOSTNAME);
  strcat(msg, "\"}}");

  // Send content to client
  server.send(200, "application/json", msg);
}


/**
   ROUTE - "/authentication/logout"
   @param void
   @return void
*/
void handleRoute_authentication_logout() {
  destroySession();
  server.sendHeader(F("Location"), F("/?status=logged-out"));
  server.send(302, "text/html", "Logging out...");
}


/**
   ROUTE - "/system/set-wifi"
   @param void
   @return void
*/
void handleRoute_newWifiDetails() {
  if (checkCookieAuthed()) {
    
    char msg[2000];
    strcpy(msg, "<html>");
    strcat(msg, "  <head>");
    strcat(msg, "    <title>");  strcat(msg, HOSTNAME);  strcat(msg, " - ESP32 SENSOR</title>\r\n");
    strcat(msg, "    <style type=\"text/css\">\r\n");
    strcat(msg, "      body  { background-color:#555; font-size:16px; color:#444; font-family: Sans-Serif; box-sizing: border-box; }\r\n");
    strcat(msg, "      main  { margin:5% auto; width:100%; max-width:360px; padding:30px 40px; background-color:#eee; border-radius:10px; box-shadow:0 0 20px #222; }\r\n");
    strcat(msg, "      h1    { font-size:24px; font-weight:bold; color:#D52E84; }\r\n");
    strcat(msg, "      p     { font-size:16px; font-weight:normal; }\r\n");
    strcat(msg, "      a     { color:#D52E84; } a:visited { color:#D52E84; }\r\n");
    strcat(msg, "      input { border:2px solid #bbb; color:#444; border-radius:5px; padding:5px; }\r\n");
    strcat(msg, "    </style>\r\n");
    strcat(msg, "  </head>");
    strcat(msg, "  <body>");
    strcat(msg, "    <main>");
    strcat(msg, "  <h1>Set WIFI Details</h1>\r\n");
    strcat(msg, "  <p>");
    strcat(msg, "    <h3>Current Details:</h3> '"); strcat(msg, SSIDNAME); strcat(msg, "' / '"); strcat(msg, SSIDPASS); strcat(msg, "'.");
    strcat(msg, "  </p>");
    strcat(msg, "  <p>");
    strcat(msg, "    <form action=\"/system/set-wifi/save\" method=\"POST\">");
    strcat(msg, "      <h3>SSID Name</h3>");
    strcat(msg, "      <input type=\"text\" name=\"ssid\"><br>");
    strcat(msg, "      <h3>SSID Password</h3>");
    strcat(msg, "      <input type=\"password\" name=\"password\"><br><br>");
    strcat(msg, "      <input type=\"submit\" value=\"Save!\"><br>");
    strcat(msg, "    </form>");
    strcat(msg, "  </p>");
    strcat(msg, "  <p>");
    strcat(msg, "    <a href='/dashboard'>[x] Back</a><br>");
    strcat(msg, "  </p>");
    strcat(msg, "    </main>");
    strcat(msg, "  </body>");
    strcat(msg, "</html>");
  
    // Send content to client
    setHeaders_NoCache();
    setHeaders_CrossOrigin();
    server.send(200, "text/html", msg);
  }
}


/**
   ROUTE - "/system/new-wifi/save"
   @param void
   @return void
*/
void handleRoute_newWifiDetails_Save() {
  if (checkCookieAuthed()) {
    
    char msgResponse[255];
    String inSSID     = server.arg("ssid");
    String inPASSWORD = server.arg("password");
    
    // Check if set then save
    if (inSSID == "" || inPASSWORD == "") {
      strcat(msgResponse, "Details not saved because at least one of them was not provided.");
    
    // Save it
    } else {
  
      // Convert to char array
      char inSSIDc[30];
      char inPASSWORDc[30];
      strcpy(inSSIDc, inSSID.c_str());
      strcpy(inPASSWORDc, inPASSWORD.c_str());
  
      // Save details
      setWifiCredentials(inSSIDc, inPASSWORDc);
      strcpy(msgResponse, "Saved.");
    }
    
    char msg[2000];
    strcpy(msg, "<html>");
    strcat(msg, "  <head>");
    strcat(msg, "    <title>");  strcat(msg, HOSTNAME);  strcat(msg, " - ESP32 SENSOR</title>\r\n");
    strcat(msg, "    <style type=\"text/css\">\r\n");
    strcat(msg, "      body  { background-color:#555; font-size:16px; color:#444; font-family: Sans-Serif; box-sizing: border-box; }\r\n");
    strcat(msg, "      main  { margin:5% auto; width:100%; max-width:360px; padding:30px 40px; background-color:#eee; border-radius:10px; box-shadow:0 0 20px #222; }\r\n");
    strcat(msg, "      h1    { font-size:24px; font-weight:bold; color:#D52E84; }\r\n");
    strcat(msg, "      p     { font-size:16px; font-weight:normal; }\r\n");
    strcat(msg, "      a     { color:#D52E84; } a:visited { color:#D52E84; }\r\n");
    strcat(msg, "      input { border:2px solid #bbb; color:#444; border-radius:5px; padding:5px; }\r\n");
    strcat(msg, "    </style>\r\n");
    strcat(msg, "  </head>");
    strcat(msg, "  <body>");
    strcat(msg, "    <main>");
    strcat(msg, "  <h1>Set WIFI Details</h1>\r\n");
    strcat(msg, "  <p>");
    strcat(msg, "    "); strcat(msg, msgResponse);
    strcat(msg, "  </p>");
    strcat(msg, "  <p>");
    strcat(msg, "    <a href='/dashboard'>[x] Back</a><br>");
    strcat(msg, "  </p>");
    strcat(msg, "    </main>");
    strcat(msg, "  </body>");
    strcat(msg, "</html>");
  
    // Send content to client
    setHeaders_NoCache();
    setHeaders_CrossOrigin();
    server.send(200, "text/html", msg);
  }
}


/**
   ROUTE - "/system/set-hostname"
   @param void
   @return void
*/
void handleRoute_newHostname() {
  if (checkCookieAuthed()) {
    
    char msg[2000];
    strcpy(msg, "<html>");
    strcat(msg, "  <head>");
    strcat(msg, "    <title>");  strcat(msg, HOSTNAME);  strcat(msg, " - ESP32 SENSOR</title>\r\n");
    strcat(msg, "    <style type=\"text/css\">\r\n");
    strcat(msg, "      body  { background-color:#555; font-size:16px; color:#444; font-family: Sans-Serif; box-sizing: border-box; }\r\n");
    strcat(msg, "      main  { margin:5% auto; width:100%; max-width:360px; padding:30px 40px; background-color:#eee; border-radius:10px; box-shadow:0 0 20px #222; }\r\n");
    strcat(msg, "      h1    { font-size:24px; font-weight:bold; color:#D52E84; }\r\n");
    strcat(msg, "      p     { font-size:16px; font-weight:normal; }\r\n");
    strcat(msg, "      a     { color:#D52E84; } a:visited { color:#D52E84; }\r\n");
    strcat(msg, "      input { border:2px solid #bbb; color:#444; border-radius:5px; padding:5px; }\r\n");
    strcat(msg, "    </style>\r\n");
    strcat(msg, "  </head>");
    strcat(msg, "  <body>");
    strcat(msg, "    <main>");
    strcat(msg, "  <h1>Set Hostname</h1>\r\n");
    strcat(msg, "  <p>");
    strcat(msg, "    <h3>Current Details</h3>'"); strcat(msg, HOSTNAME); strcat(msg, "'.");
    strcat(msg, "  </p>");
    strcat(msg, "  <p>");
    strcat(msg, "    <form action=\"/system/set-hostname/save\" method=\"POST\">");
    strcat(msg, "      <h3>HOSTNAME</h3>");
    strcat(msg, "      <input type=\"text\" name=\"hostname\"><br>");
    strcat(msg, "      <input type=\"submit\" value=\"Save!\"><br>");
    strcat(msg, "    </form>");
    strcat(msg, "  </p>");
    strcat(msg, "  <p>");
    strcat(msg, "    <a href='/dashboard'>[x] Back</a><br>");
    strcat(msg, "  </p>");
    strcat(msg, "    </main>");
    strcat(msg, "  </body>");
    strcat(msg, "</html>");
  
    // Send content to client
    setHeaders_NoCache();
    setHeaders_CrossOrigin();
    server.send(200, "text/html", msg);
  }
}


/**
   ROUTE - "/system/set-hostname/save"
   @param void
   @return void
*/
void handleRoute_newHostname_Save() {
  if (checkCookieAuthed()) {
    
    String inHOSTNAME = server.arg("hostname");
    char msgResponse[255];
    
    // Check if set then save
    if (inHOSTNAME == "") {
      strcat(msgResponse, "Details not saved because at least one of them was not provided.");
    
    // Save it
    } else {
  
      // Covert to char array
      char inHOSTNAMEc[30];
      strcpy(inHOSTNAMEc, inHOSTNAME.c_str());
  
      // Set the hostname
      setHostname(inHOSTNAMEc);
      strcpy(msgResponse, "Saved.");
    }
    
    char msg[2000];
    strcpy(msg, "<html>");
    strcat(msg, "  <head>");
    strcat(msg, "    <title>");  strcat(msg, HOSTNAME);  strcat(msg, " - ESP32 SENSOR</title>\r\n");
    strcat(msg, "    <style type=\"text/css\">\r\n");
    strcat(msg, "      body  { background-color:#555; font-size:16px; color:#444; font-family: Sans-Serif; box-sizing: border-box; }\r\n");
    strcat(msg, "      main  { margin:5% auto; width:100%; max-width:360px; padding:30px 40px; background-color:#eee; border-radius:10px; box-shadow:0 0 20px #222; }\r\n");
    strcat(msg, "      h1    { font-size:24px; font-weight:bold; color:#D52E84; }\r\n");
    strcat(msg, "      p     { font-size:16px; font-weight:normal; }\r\n");
    strcat(msg, "      a     { color:#D52E84; } a:visited { color:#D52E84; }\r\n");
    strcat(msg, "      input { border:2px solid #bbb; color:#444; border-radius:5px; padding:5px; }\r\n");
    strcat(msg, "    </style>\r\n");
    strcat(msg, "  </head>");
    strcat(msg, "  <body>");
    strcat(msg, "    <main>");
    strcat(msg, "  <h1>Set Hostname</h1>\r\n");
    strcat(msg, "  <p>");
    strcat(msg, "    "); strcat(msg, msgResponse);
    strcat(msg, "  </p>");
    strcat(msg, "  <p>");
    strcat(msg, "    <a href='/dashboard'>[x] Back</a><br>");
    strcat(msg, "  </p>");
    strcat(msg, "    </main>");
    strcat(msg, "  </body>");
    strcat(msg, "</html>");
  
    // Send content to client
    setHeaders_NoCache();
    setHeaders_CrossOrigin();
    server.send(200, "text/html", msg);
  }
}


/**
   ROUTE - "/system/set-mode"
   @param void
   @return void
*/
void handleRoute_newMode() {
  if (checkCookieAuthed()) {
    
    char msg[2000];
    strcpy(msg, "<html>");
    strcat(msg, "  <head>");
    strcat(msg, "    <title>");  strcat(msg, HOSTNAME);  strcat(msg, " - ESP32 SENSOR</title>\r\n");
    strcat(msg, "    <style type=\"text/css\">\r\n");
    strcat(msg, "      body  { background-color:#555; font-size:16px; color:#444; font-family: Sans-Serif; box-sizing: border-box; }\r\n");
    strcat(msg, "      main  { margin:5% auto; width:100%; max-width:360px; padding:30px 40px; background-color:#eee; border-radius:10px; box-shadow:0 0 20px #222; }\r\n");
    strcat(msg, "      h1    { font-size:24px; font-weight:bold; color:#D52E84; }\r\n");
    strcat(msg, "      p     { font-size:16px; font-weight:normal; }\r\n");
    strcat(msg, "      a     { color:#D52E84; } a:visited { color:#D52E84; }\r\n");
    strcat(msg, "      input { border:2px solid #bbb; color:#444; border-radius:5px; padding:5px; }\r\n");
    strcat(msg, "    </style>\r\n");
    strcat(msg, "  </head>");
    strcat(msg, "  <body>");
    strcat(msg, "    <main>");
    strcat(msg, "  <h1>Set Mode</h1>\r\n");
    strcat(msg, "  <p>");
    strcat(msg, "    <h3>Current Details</h3>'"); strcat(msg, MODE); strcat(msg, "'.");
    strcat(msg, "  </p>");
    strcat(msg, "  <p>");
    strcat(msg, "    <form action=\"/system/set-mode/save\" method=\"POST\">");
    strcat(msg, "      <h3>Select a mode:</h3>");
    strcat(msg, "      <select name=\"mode\">");
    strcat(msg, "        <option value=\"1\" default>LED Strip</option>");
    strcat(msg, "        <option value=\"2\">LED Strips with MOSFET/RELAYS</option>");
    strcat(msg, "        <option value=\"6\">Thermostat</option>");
    strcat(msg, "        <option value=\"7\">Smart Plug</option>");
    strcat(msg, "        <option value=\"8\">Home AP</option>");
    strcat(msg, "        <option value=\"3\">BMP180 Sensor</option>");
    strcat(msg, "        <option value=\"4\">BME280 Sensor</option>");
    strcat(msg, "        <option value=\"5\">Weather Station</option>");
    strcat(msg, "      </select>");
    strcat(msg, "      <input type=\"submit\" value=\"Save!\"><br>");
    strcat(msg, "    </form>");
    strcat(msg, "  </p>");
    strcat(msg, "  <p>");
    strcat(msg, "    <a href='/dashboard'>[x] Back</a><br>");
    strcat(msg, "  </p>");
    strcat(msg, "    </main>");
    strcat(msg, "  </body>");
    strcat(msg, "</html>");
  
    // Send content to client
    setHeaders_NoCache();
    setHeaders_CrossOrigin();
    server.send(200, "text/html", msg);
  }
}


/**
   ROUTE - "/system/set-mode/save"
   @param void
   @return void
*/
void handleRoute_newMode_Save() {
  if (checkCookieAuthed()) {
    
    String inMODE = server.arg("mode");
    char msgResponse[255];
    
    // Check if set then save
    if (inMODE == "") {
      strcat(msgResponse, "Details not saved because at least one of them was not provided.");
    
    // Save it
    } else {
  
      // Covert to char array
      char inMODEc[30];
      strcpy(inMODEc, inMODE.c_str());
  
      // Set the hostname
      setMode(inMODEc);
      strcpy(msgResponse, "Saved.");
    }
    
    char msg[2000];
    strcpy(msg, "<html>");
    strcat(msg, "  <head>");
    strcat(msg, "    <title>");  strcat(msg, HOSTNAME);  strcat(msg, " - ESP32 SENSOR</title>\r\n");
    strcat(msg, "    <style type=\"text/css\">\r\n");
    strcat(msg, "      body  { background-color:#555; font-size:16px; color:#444; font-family: Sans-Serif; box-sizing: border-box; }\r\n");
    strcat(msg, "      main  { margin:5% auto; width:100%; max-width:360px; padding:30px 40px; background-color:#eee; border-radius:10px; box-shadow:0 0 20px #222; }\r\n");
    strcat(msg, "      h1    { font-size:24px; font-weight:bold; color:#D52E84; }\r\n");
    strcat(msg, "      p     { font-size:16px; font-weight:normal; }\r\n");
    strcat(msg, "      a     { color:#D52E84; } a:visited { color:#D52E84; }\r\n");
    strcat(msg, "      input { border:2px solid #bbb; color:#444; border-radius:5px; padding:5px; }\r\n");
    strcat(msg, "    </style>\r\n");
    strcat(msg, "  </head>");
    strcat(msg, "  <body>");
    strcat(msg, "    <main>");
    strcat(msg, "  <h1>Set Hostname</h1>\r\n");
    strcat(msg, "  <p>");
    strcat(msg, "    "); strcat(msg, msgResponse);
    strcat(msg, "  </p>");
    strcat(msg, "  <p>");
    strcat(msg, "    <a href='/dashboard'>[x] Back</a><br>");
    strcat(msg, "  </p>");
    strcat(msg, "    </main>");
    strcat(msg, "  </body>");
    strcat(msg, "</html>");
  
    // Send content to client
    setHeaders_NoCache();
    setHeaders_CrossOrigin();
    server.send(200, "text/html", msg);
  }
}


/**
   ROUTE - "/system/wipe-eeprom"
   @param void
   @return void
*/
void handleRoute_system_wipe_eeprom() {
  if (checkCookieAuthed()) {
    
    wipeEEPROM(); delay(2000);
    server.send(200, "text/html", "EEPROM Wiped. Please reboot.");
  }
}


/**
   ROUTE - "/authentication"
   @param void
   @return void
*/
void handleRoute_authentication() {
  String username = server.arg("username");
  String password = server.arg("password");

  // Check credentials are correct, or has cookie, then login
  if ((username == APIUSER && password == APIPASS) || checkCookieAuthedBool()) {
    createSession();
    server.sendHeader(F("Location"), F("/dashboard"));
    server.send(302, "text/html", "Authorised.");

    // Else redirect back to login page
  } else {
    destroySession();
    server.sendHeader(F("Location"), F("/?status=no"));
    server.send(302, "text/html", "Not Authorised.");
  }
}

    

/**
   Set the NoCaching headers for the browser
   @param void
   @return void
*/
void setHeaders_NoCache() {
  server.sendHeader(F("Expires"),       F("-1"));
  server.sendHeader(F("Pragma"),        F("no-cache"));
  server.sendHeader(F("cache-control"), F("no-cache, no-store"));
};


/**
   Set the CORS headers for the browser
   @param void
   @return void
*/
void setHeaders_CrossOrigin() {
  server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
  server.sendHeader(F("Access-Control-Max-Age"), F("600"));
  server.sendHeader(F("Access-Control-Allow-Methods"), F("POST,GET"));
  server.sendHeader(F("Access-Control-Allow-Headers"), F("*"));
};


/**
   Creates a new SESSION
   @param void
   @return void
*/
String createSessionID () {
  char *key = "";
  char *payload = "";
  byte hmacResult[32];
  char hash[255] = "";
  int  r1, r2 = 0;
  char r3[20], r4[20] = "";
  String hashf = "";

  // Generate KEY and VALUE
  //rand(time(NULL));
  r1 = rand();
  r2 = rand();
  sprintf (r3, "%i", r1);
  sprintf (r4, "%i", r2);
  key     = r3;
  payload = r4;

  // Generate HASH String
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

  const size_t payloadLength = strlen(payload);
  const size_t keyLength = strlen(key);

  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char *) key, keyLength);
  mbedtls_md_hmac_update(&ctx, (const unsigned char *) payload, payloadLength);
  mbedtls_md_hmac_finish(&ctx, hmacResult);
  mbedtls_md_free(&ctx);

  for (int i = 0; i < sizeof(hmacResult); i++) {
    char str[3];
    sprintf(str, "%02x", (int)hmacResult[i]);
    hashf = hashf + str;
  }
  return hashf;
}


/**
   Create Session
   @param void
   @return void
*/
void createSession() {
  String hash = createSessionID();
  String cookie = "X-SESSION=" + hash;
  server.sendHeader("Set-Cookie", cookie);
  SESSION_COOKIE_KEY = cookie;
}


/**
   Checks if authed with cookie, then redirects to login if not.
   @param void
   @return void
*/
void destroySession () {
  destroySessionCookie();
  SESSION_COOKIE_KEY = "none";
}


/**
   Destroys the SESSION
   @param void
   @return void
*/
void destroySessionCookie () {
  String hash = createSessionID();
  SESSION_COOKIE_KEY = hash;
  server.sendHeader("Set-Cookie", "X-SESSION=none");
}


/**
   Checks if authed with cookie, then redirects to login if not.
   @param void
   @return void
*/
bool checkCookieAuthed () {
  String cookie_value = "09v2n548n243";
  if (server.hasHeader("Cookie") && SESSION_COOKIE_KEY != "")
    cookie_value = server.header("Cookie");
  if (cookie_value != SESSION_COOKIE_KEY) {
    server.sendHeader(F("Location"), F("/?status=no"));
    server.send(302, "text/html", "Not Authorised.");
    return false;
  }
  return true;
}


/**
   Checks if authed with cookie, then returns true/false.
   @param void
   @return void
*/
bool checkCookieAuthedBool () {
  Serial.println("Checking Cookie Authed (bool)...");
  String cookie_value = "768fh89as7d6f";
  if (server.hasHeader("Cookie") && SESSION_COOKIE_KEY != "")
    cookie_value = server.header("Cookie");
  if (cookie_value == SESSION_COOKIE_KEY)
    return true;
  return false;
}


/**
 * Gets the SSID Credentials from the EEPROM
 * @param void
 * @return void
 */
bool getWiFiCedentials () {
  int isSSIDNAMEset = EEPROM.read(EEPROM_SSIDNAME_ISSET_LOCATION);
  int isSSIDPASSset = EEPROM.read(EEPROM_SSIDPASS_ISSET_LOCATION);
  int digit1, digit2;

  // If the ISSET flag is set == 1 in the EEPROM then
  if (isSSIDNAMEset == 1 && isSSIDPASSset == 1) {
  
    // Get SSID NAME
    for (int i=EEPROM_SSIDNAME_LOCATION; digit1 != 0; i++) {

      // Read EEPROM value
      digit1 = EEPROM.read(i);
      
      // If digit is 0 then skip (0 is end of string)
      if (digit1 == 0)
        continue;

      // Put value into the global one char at a time as you read from the EEPROM
      SSIDNAME[i-EEPROM_SSIDNAME_LOCATION] = eepromNumberToChar(digit1); // Decode the digit to the Char it represents
    }
    
    // Get SSID PASS
    for (int i=EEPROM_SSIDPASS_LOCATION; digit2 != 0; i++) {

      // Read EEPROM value
      digit2 = EEPROM.read(i);
      
      // If digit is 0 then skip (0 is end of string)
      if (digit2 == 0)
        continue;

      // Put value into the global one char at a time as you read from the EEPROM
      SSIDPASS[i-EEPROM_SSIDPASS_LOCATION] = eepromNumberToChar(digit2); // Decode the digit to the Char it represents
    }

    return true;
  }
  
  // Else, run setup program
  return false;
}


/**
 * Gets the HOSTNAME from the EEPROM
 * @param void
 * @return void
 */
bool getHostname () {
  int isHostnameSet = EEPROM.read(EEPROM_HOSTNAME_ISSET_LOCATION);
  int digit;
  
  // If the ISSET flag is set == 1 in the EEPROM then
  if (isHostnameSet == 1) {

    // Read out the HOSTNAME from the EEPROM
    for (int i=EEPROM_HOSTNAME_LOCATION; digit != 0; i++) {

      // Read EEPROM value
      digit = EEPROM.read(i);
      
      // If digit is 0 then skip (0 is end of string)
      if (digit == 0)
        continue;
      
      HOSTNAME[i-EEPROM_HOSTNAME_LOCATION] = eepromNumberToChar(digit); // Decode the digit to the Char it represents
    }
    return true;
  }
  // Else use default name
  return false;
}


/**
 * Gets the MODE from the EEPROM
 * @param void
 * @return void
 */
bool getMode () {
  int isModeSet = EEPROM.read(EEPROM_MODE_ISSET_LOCATION);
  int digit;
  
  // If the ISSET flag is set == 1 in the EEPROM then
  if (isModeSet == 1) {

    // Read out the HOSTNAME from the EEPROM
    for (int i=EEPROM_MODE_LOCATION; digit != 0; i++) {

      // Read EEPROM value
      digit = EEPROM.read(i);
      
      // If digit is 0 then skip (0 is end of string)
      if (digit == 0)
        continue;
      
      MODE[i-EEPROM_MODE_LOCATION] = eepromNumberToChar(digit); // Decode the digit to the Char it represents
    }
    return true;
  }
  // Else use default name
  return false;
}


/**
 * Sets the SSID Credentials in the EEPROM
 * @param void
 * @return void
 */
bool setWifiCredentials (char tmpSSID[30], char tmpPASS[30]) {
  Serial.println("> SAVING WIFI CREDENTIALS...");
  
  // WIPE AL SAVED DATA FROM FIELDS in EPPROM
  for (int i=0; i<30; i++) {
    EEPROM.write(EEPROM_SSIDNAME_LOCATION+i, 0);
    EEPROM.write(EEPROM_SSIDPASS_LOCATION+i, 0);
  }
  EEPROM.write(EEPROM_SSIDNAME_ISSET_LOCATION, 0);
  EEPROM.write(EEPROM_SSIDPASS_ISSET_LOCATION, 0);
  EEPROM.commit();
  
  // Set SSID NAME
  for (int i=0; tmpSSID[i] != '\0'; i++) {
    EEPROM.write(EEPROM_SSIDNAME_LOCATION+i, eepromCharToNumber(tmpSSID[i]));      // Set the digit in the EEPROM 
  }
  
  // Set SSID PASS
  for (int i=0; tmpPASS[i] != '\0'; i++) {
    EEPROM.write(EEPROM_SSIDPASS_LOCATION+i, eepromCharToNumber(tmpPASS[i]));      // Set the digit in the EEPROM
  }

  // Save the ISSET fields in EEPROM
  EEPROM.write(EEPROM_SSIDNAME_ISSET_LOCATION, 1);
  EEPROM.write(EEPROM_SSIDPASS_ISSET_LOCATION, 1);
  
  // Commit EEPROM Save!
  EEPROM.commit();
  return true;
}


/**
 * Sets the HOSTNAME in the EEPROM
 * @param void
 * @return void
 */
bool setHostname (char tmpHOSTNAME[30]) {
  Serial.println("> SAVING HOSTNAME...");
  
  // WIPE AL SAVED DATA FROM FIELDS in EPPROM
  for (int i=0; i<30; i++) {
    EEPROM.write(EEPROM_HOSTNAME_LOCATION+i, 0);
  }
  EEPROM.write(EEPROM_HOSTNAME_ISSET_LOCATION, 0);
  EEPROM.commit();
    
  // Set hostname
  for (int i=0; tmpHOSTNAME[i] != '\0'; i++) {
    EEPROM.write(EEPROM_HOSTNAME_LOCATION+i, eepromCharToNumber(tmpHOSTNAME[i]));      // Set the digit in the EEPROM
  }
  
  // Save the ISSET fields in EEPROM
  EEPROM.write(EEPROM_HOSTNAME_ISSET_LOCATION, 1);
  
  // Commit EEPROM Save!
  EEPROM.commit();
  return true;
}


/**
 * Sets the HOSTNAME in the EEPROM
 * @param void
 * @return void
 */
bool setMode (char tmpMODE[30]) {
  Serial.println("> SAVING HOSTNAME...");
  
  // WIPE AL SAVED DATA FROM FIELDS in EPPROM
  for (int i=0; i<30; i++) {
    EEPROM.write(EEPROM_MODE_LOCATION+i, 0);
  }
  EEPROM.write(EEPROM_MODE_ISSET_LOCATION, 0);
  EEPROM.commit();
    
  // Set hostname
  for (int i=0; tmpMODE[i] != '\0'; i++) {
    EEPROM.write(EEPROM_MODE_LOCATION+i, eepromCharToNumber(tmpMODE[i]));      // Set the digit in the EEPROM
  }
  
  // Save the ISSET fields in EEPROM
  EEPROM.write(EEPROM_MODE_ISSET_LOCATION, 1);
  
  // Commit EEPROM Save!
  EEPROM.commit();
  return true;
}


/**
 * Wipes the entire EEPROM
 * @param void
 * @return void
 */
void wipeEEPROM () {
  for (int i=0; i<256; i++) {
    EEPROM.write(i, 0);
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
}


/**
 * Encodes Chars into Ints for the EEPROM
 * (EEPROM only stores ints 0-255)
 * @param char inString[30]
 * @return int*
 */
int eepromCharToNumber (char inChar) {
  char chars[100] = {" abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@??$%^&*-_=+~#/?>.<,|;:'"};
  char selLibChar;
  
  // Now search the library array for that char
  for (int i=0; i<89; i++) {
    selLibChar = chars[i];

    // If the char given and the char in the library match, add the
    // library position to the out array as a value;
    if (inChar == selLibChar) {
      return i;
    }
  }
  return 0;
}


/**
 * Decodes Int into Char
 * (EEPROM only stores ints 0-255)
 * @param char inString[30]
 * @return int*
 */
char eepromNumberToChar (int number) {
  char chars[100] = {" abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@??$%^&*-_=+~#/?>.<,|;:'"};
  return chars[number];
}


/** 
 * Avg Float Value from Two Given
 * @param float d1
 * @param float d2
 * @param float d3
 * @return float
 */
float getAvg (float d1, float d2, float d3) {
  return ((d1+d2+d3) / 3);
}


/**
   Reas the sensor and return the value
   @param String sensor
   @param float r
   @return char
*/
float readSensor (String sensor) {
    
    float r = 0.00;
    char r1[10];
  
    // read sensor
    for (int i = 0; i < 40; i++) {
  
      // TEMPERATURE SENSOR
      if (sensor == "temperature1") {
        r = bmp.readTemperature();
        if (r > -20 && r < 40)
          break;
  
      // PRESSURE SENSOR
      } else if (sensor == "pressure") {
        r = bmp.readPressure();
        if (r > 850 && r < 120000)
          break;

      // DHT11-1 - HUMIDITY
      } else if (sensor == "humidity1") {
        r = dhtA.readHumidity();
        break;

      // DHT11-1 - TEMPERATURE
      } else if (sensor == "temperature2") {
        r = dhtA.readTemperature();
        break;

      // DHT11-2 - HUMIDITY
      } else if (sensor == "humidity2") {
        r = dhtB.readHumidity();
        break;

      // DHT11-2 - TEMPERATURE
      } else if (sensor == "temperature3") {
        r = dhtB.readTemperature();
        break;
      }
      
      delay(50);
    }
    return r;
}
