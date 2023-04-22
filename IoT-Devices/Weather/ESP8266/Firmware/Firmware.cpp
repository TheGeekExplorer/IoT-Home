#include <stdlib.h>
#include <Wire.h>
#include <time.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h> 
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>   // Include the WebServer library
#include <EEPROM.h>
#include "mbedtls/md.h"
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include "DHT.h"


/** SENSORS & SENSOR CONFIG **/

// BMP180 - Temperature #1 & #2, Air Pressure #1 & #2, Altitude #1 & #2
Adafruit_BMP085 bmp;
#define ALTITUDE 65.0 // Altitude in meters - Manchester

// DHT11 - Humidity #1 & #2, and Temperature #3 & #4
#define DHTTYPE DHT11          // DHT11 - Define type as DHT11
uint8_t DHTPin1 = 14;          // DHT11 - Define pins used by #1 and #2 sensors (D5 and D4)
uint8_t DHTPin2 = 16;          // DHT11 - Define pins used by #1 and #2 sensors (D5 and D4)
DHT dht1(DHTPin1, DHTTYPE);    // DHT11 - Instantiate DHT11 library as Sensor 1
DHT dht2(DHTPin2, DHTTYPE);    // DHT11 - Instantiate DHT11 library as Sensor 2

// CO2 - MQ135 - Gas Sensor
#define CO2Pin_1 A0           // CO2 / MQ-135 / Gas Sensor


/** SYSTEM VARIABLES & CONFIG **/

// WiFi Config Variables
char SSIDNAME[30] = "wifi name here";
char SSIDPASS[30] = "password here";
char HOSTNAME[30];
char     MODE[1];
//ESP8266WiFiMulti wifiMulti;     // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

// Set your Static IP address
IPAddress local_IP(172, 16, 1, 16);
IPAddress gateway(172, 16, 1, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

// API Config
char APIUSER[20] = "admin";
char APIPASS[20] = "password";

// EEPROM Config
#define EEPROM_SIZE 255
int EEPROM_SSIDNAME_LOCATION         = 1;
int EEPROM_SSIDPASS_LOCATION         = 36;
int EEPROM_HOSTNAME_LOCATION         = 71;
int EEPROM_MODE_LOCATION             = 120;
int EEPROM_SSIDNAME_ISSET_LOCATION   = 111;
int EEPROM_SSIDPASS_ISSET_LOCATION   = 113;
int EEPROM_HOSTNAME_ISSET_LOCATION   = 115;
int EEPROM_MODE_ISSET_LOCATION       = 116;

// Device Capabilities Flags in EEPROM (e.g. has DHT11 attached etc)
int EEPROM_CAPABILITIES_TEMERATURE_1   = 120;
int EEPROM_CAPABILITIES_TEMERATURE_2   = 121;
int EEPROM_CAPABILITIES_HUMIDITY_1     = 122;
int EEPROM_CAPABILITIES_HUMIDITY_2     = 123;
int EEPROM_CAPABILITIES_AIR_PRESSURE_1 = 124;
int EEPROM_CAPABILITIES_AIR_PRESSURE_2 = 125;
int EEPROM_CAPABILITIES_ALTITUDE_1     = 126;
int EEPROM_CAPABILITIES_ALTITUDE_2     = 127;
int EEPROM_CAPABILITIES_CO2_1          = 128;
int EEPROM_CAPABILITIES_CO2_2          = 129;
int EEPROM_CAPABILITIES_LIGHT_1        = 130;
int EEPROM_CAPABILITIES_LIGHT_2        = 131;
int EEPROM_CAPABILITIES_RAIN_1         = 130;
int EEPROM_CAPABILITIES_RAIN_2         = 131;

// Has Capability of this or not?  (Stores Blank/0/1)
int CAPABILITIES_TEMERATURE_1   = 0;
int CAPABILITIES_TEMERATURE_2   = 0;
int CAPABILITIES_HUMIDITY_1     = 0;
int CAPABILITIES_HUMIDITY_2     = 0;
int CAPABILITIES_AIR_PRESSURE_1 = 0;
int CAPABILITIES_AIR_PRESSURE_2 = 0;
int CAPABILITIES_ALTITUDE_1     = 0;
int CAPABILITIES_ALTITUDE_2     = 0;
int CAPABILITIES_CO2_1          = 0;
int CAPABILITIES_CO2_2          = 0;
int CAPABILITIES_LIGHT_1        = 0;
int CAPABILITIES_LIGHT_2        = 0;
int CAPABILITIES_RAIN_1         = 0;
int CAPABILITIES_RAIN_2         = 0;

// Other Variables
String SESSION_COOKIE_KEY = "none";
String header;
int relayOnePin = 26;

// WEB SERVER - Default route prototypes
char html_header[2000];
char html_footer[1000];
void handleRoot();              // function prototypes for HTTP handlers
void handleNotFound();
ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80


/**
 * Setup the Microcontroller - ESP8266
 * @param void
 * @return void
 */
void setup() {

  // SERIAL - Setup serial
  Serial.begin(9600);
  Serial.println("> Serial Started.");
  delay(1000);  // If no HOSTNAME configured then use default
  
  
  /**  EEPROM OPERATIONS  **/
  EEPROM.begin(EEPROM_SIZE);
  bool isCreds    = getWiFiCedentials();  // Gets SSID / SSID PASS from EEPROM and stores in SSIDNAME / SSIDPASS globals.
  bool isHostname = getHostname();        // Gets HOSTNAME from EEPROM and stores in HOSTNAME global.

  // If no hostname set then set a default name
  if (!isHostname)
    strcat(HOSTNAME, "X-SENSOR");

  
  /**  WIFI SETUP & OPERATIONS  **/
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);  // Set Static IP and other details
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  WiFi.begin(SSIDNAME, SSIDPASS);                        // Connect to the Wifi
  
  // Wait for connect...
  Serial.print("> WIFI: Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  // Connected to Wifi...
  WiFi.setSleep(false);
  WiFi.setHostname(HOSTNAME);
  Serial.print(">> Device IP: ");   Serial.println(WiFi.localIP());
  Serial.print(">> MAC Address:" ); Serial.println(WiFi.macAddress());
  Serial.print(">> SSID:" );        Serial.println(WiFi.SSID());
  Serial.print(">> RSSI:" );        Serial.println(WiFi.RSSI());

  // Wait for mDNS to Startup...
  if (MDNS.begin("esp8266")) {              // Start the mDNS responder for esp8266.local
    Serial.println("> MDNS: mDNS responder started.");
  } else {
    Serial.println("> MDNS: ! Error setting up MDNS responder!");
  }
  

  // SERVER - SYSTEM ROUTES
  server.onNotFound(handleNotFound);        // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"
  server.on("/",                               HTTP_GET,  handleRoute_root);
  server.on("/authentication",                 HTTP_POST, handleRoute_authentication);
  server.on("/authentication/logout",          HTTP_GET,  handleRoute_authentication_logout);
  server.on("/dashboard",                      HTTP_GET,  handleRoute_dashboard);
  server.on("/system/wipe-eeprom",             HTTP_GET,  handleRoute_system_wipe_eeprom);
  server.on("/system/set-wifi",                HTTP_GET,  handleRoute_newWifiDetails);
  server.on("/system/set-wifi/save",           HTTP_POST, handleRoute_newWifiDetails_Save);
  server.on("/system/set-hostname",            HTTP_GET,  handleRoute_newHostname);
  server.on("/system/set-hostname/save",       HTTP_POST, handleRoute_newHostname_Save);
  server.on("/system/capabilities",            HTTP_GET,  handleRoute_capabilities);
  server.on("/system/capabilities/save",       HTTP_POST, handleRoute_capabilities_Save);

  // API ROUTES
  server.on("/api/v1/identity",                HTTP_GET,  handleRoute_api_identity);
  server.on("/api/v1/capabilities",            HTTP_GET,  handleRoute_api_capabilities);
  server.on("/api/v1/temperature/1",           HTTP_GET,  handleRoute_api_temperature1);
  server.on("/api/v1/temperature/2",           HTTP_GET,  handleRoute_api_temperature2);
  server.on("/api/v1/humidity/1",              HTTP_GET,  handleRoute_api_humidity1);
  server.on("/api/v1/humidity/2",              HTTP_GET,  handleRoute_api_humidity2);
  server.on("/api/v1/co2/1",                   HTTP_GET,  handleRoute_api_co2_1);
  server.on("/api/v1/air-pressure/1",          HTTP_GET,  handleRoute_api_air_pressure_1);


  // WEB SERVER - Define which request headers you need access to
  const char *headers[] = {"Host", "Referer", "Cookie"};
  size_t headersCount = sizeof(headers) / sizeof(char*);
  server.collectHeaders(headers, headersCount);


  // SERVER - Start the server!
  setTemplates();
  server.begin();

  Serial.println("> WEB SERVER: API Server Started.");
  delay(1000);


  /** SENSORS & DEVICES **/
  
  // Read Capabiltiies from EEPROM into Global (e.g. Temperature, Humidity, CO2, etc)
  loadDeviceCapabilities();

  // #1 - BMP180 -- Temperature #1, Air Pressure, and Altitude
  //Wire.begin (4, 5);
  if (!bmp.begin()) 
  {
    Serial.println("> ERROR - Could not find BMP180 or BMP085 sensor at 0x77");
  }
  
  // #2 - DHT11 -- Humidity & Temperature #2
  pinMode(DHTPin1, INPUT);
  dht1.begin();
  pinMode(DHTPin2, INPUT);
  dht2.begin();
}


/**
 * Main Loop
 * @param void
 * @return void
 */
void loop() {
  server.handleClient();
}


// Set HEADER and FOOTER Templates
void setTemplates() {
  // Header
  strcpy(html_header, " "); 
  strcat(html_header, "<html>");
  strcat(html_header, "  <head>");
  strcat(html_header, "    <title>");  strcat(html_header, HOSTNAME);  strcat(html_header, "</title>\r\n");
  strcat(html_header, "    <style type=\"text/css\">\r\n");
  strcat(html_header, "      body  { background-color:#333; font-size:16px; color:#444; font-family: Sans-Serif; box-sizing: border-box; }"); 
  strcat(html_header, "      main  { margin:5% auto; width:100%; max-width:360px; padding:30px 40px; background-color:#eee; border-radius:10px; box-shadow:0 0 20px #222; }\r\n");
  strcat(html_header, "      h1    { font-size:24px; font-weight:bold; color:#D52E84; }\r\n");
  strcat(html_header, "      p     { font-size:16px; font-weight:normal; }\r\n");
  strcat(html_header, "      a     { color:#D52E84; } a:visited { color:#D52E84; }\r\n");
  strcat(html_header, "      input { border:2px solid #bbb; color:#444; border-radius:5px; padding:5px; }\r\n");
  strcat(html_header, "    </style>\r\n");
  strcat(html_header, "  </head>");
  strcat(html_header, "  <body>");
  
  // Footer
  strcpy(html_footer, " ");
  strcat(html_footer, " </body>");
  strcat(html_footer, "</html>");
}


// ROUTE - Home
void handleRoute_root() {
  Serial.println("> WEB SERVER: Page Request 'Home'.");

  char msg[2000];
  strcpy(msg, " "); strcat(msg, html_header);
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
  strcat(msg, html_footer);

  // Send content to client
  setHeaders_NoCache();
  setHeaders_CrossOrigin();
  server.send(200, "text/html", msg);
}


// ROUTE - NotFound
void handleNotFound(){
  Serial.println("> WEB SERVER: Page Request 'Not Found'.");
  
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}


/**
   ROUTE - "/dashboard/"
   @param void
   @return void
*/
void handleRoute_dashboard() {
  Serial.println("> WEB SERVER: Page Request 'Dashboard'.");
  
  if (checkCookieAuthed()) {
    String msg;

    msg =  html_header;
    msg += "    <main>";
    msg += "<h1>DASHBOARD<h1>";
    msg += "<h3>API Endpoints<h3>";
    msg += "<p>";
    
    // READ CAPABILITIES FROM EEPROM
    if (CAPABILITIES_TEMERATURE_1 == 1)    msg += "<a href='/api/v1/temperature/1'>/api/v1/temperature/1</a><br>";
    if (CAPABILITIES_TEMERATURE_2 == 1)    msg += "<a href='/api/v1/temperature/2'>/api/v1/temperature/2</a><br>";
    if (CAPABILITIES_HUMIDITY_1 == 1)      msg += "<a href='/api/v1/humidity/1'>/api/v1/humidity/1</a><br>";
    if (CAPABILITIES_HUMIDITY_2 == 1)      msg += "<a href='/api/v1/humidity/2'>/api/v1/humidity/2</a><br>";
    if (CAPABILITIES_AIR_PRESSURE_1 == 1)  msg += "<a href='/api/v1/air-pressure/1'>/api/v1/air-pressure/1</a><br>";
    if (CAPABILITIES_AIR_PRESSURE_2 == 1)  msg += "<a href='/api/v1/air-pressure/2'>/api/v1/air-pressure/2</a><br>";
    if (CAPABILITIES_ALTITUDE_1 == 1)      msg += "<a href='/api/v1/altitude/1'>/api/v1/altitude/1</a><br>";
    if (CAPABILITIES_ALTITUDE_2 == 1)      msg += "<a href='/api/v1/altitude/2'>/api/v1/altitude/2</a><br>";
    if (CAPABILITIES_CO2_1 == 1)           msg += "<a href='/api/v1/co2/1'>/api/v1/co2/1</a><br>";
    if (CAPABILITIES_CO2_2 == 1)           msg += "<a href='/api/v1/co2/2'>/api/v1/co2/2</a><br>";
    if (CAPABILITIES_LIGHT_1 == 1)         msg += "<a href='/api/v1/light/1'>/api/v1/light/1</a><br>";
    if (CAPABILITIES_LIGHT_2 == 1)         msg += "<a href='/api/v1/light/2'>/api/v1/light/2</a><br>";
    if (CAPABILITIES_RAIN_1 == 1)          msg += "<a href='/api/v1/rain/1'>/api/v1/rain/1</a><br>";
    if (CAPABILITIES_RAIN_2 == 1)          msg += "<a href='/api/v1/rain/2'>/api/v1/rain/2</a><br>";

    msg += "</p>";
    msg += "<h3>System<h3>";
    msg += "<p>";
    msg += "  <a href='/system/set-mode'>Set Device Mode</a><br>";
    msg += "  <a href='/system/set-wifi'>Set new Wifi Details</a><br>";
    msg += "  <a href='/system/set-hostname'>Set New Hostname</a><br>";
    msg += "  <a href='/system/capabilities'>Set Capabiltiies</a><br>";
    msg += "  <a href='/authentication/logout'>[x] Logout</a><br>";
    msg += "</p>";
    msg += "<h3>Danger Zone<h3>";
    msg += "<p>";
    msg += "  <a href='/system/wipe-eeprom'>Wipe EEPROM</a><br>";
    msg += "</p><br>";
    msg += "<p>";
    msg += "  <a href='/authentication/logout'>[x] Logout</a><br>";
    msg += "</p>";

    // FOOTER
    msg += "</main>";
    msg += html_footer;
  
    // Send content to client
    setHeaders_NoCache();
    setHeaders_CrossOrigin();
    server.send(200, "text/html", msg);
  }
}


/**
   ROUTE - "/api/v1/identity"
   Exists so that it can be identifed on the network because
   we cannot rely on the IP address as it changes.
   @param void
   @return void
*/
void handleRoute_api_identity() {
  Serial.println("> WEB SERVER: Page Request 'API Identity'.");
  
  setHeaders_NoCache();
  setHeaders_CrossOrigin();

  // Define identity
  String msg;
  
  msg = "{\"result\":{\"ROUTE\":\"/api/v1/identity\",\"status\":\"OK\", \"value\":\"";
  msg += HOSTNAME;
  msg += "\"}}";

  // Send content to client
  server.send(200, "application/json", msg);
}


/**
   ROUTE - "/api/v1/capabilities"
   @param void
   @return void
*/
void handleRoute_api_capabilities() {
  Serial.println("> WEB SERVER: Page Request 'Capabilities'.");
  
  setHeaders_NoCache();
  setHeaders_CrossOrigin();

  // Define identity
  String msg;
  String caps = "API";

  // READ CAPABILITIES FROM EEPROM
  if (CAPABILITIES_TEMERATURE_1 == 1)    caps = caps + ",TEMPERATURE_1";
  if (CAPABILITIES_TEMERATURE_2 == 1)    caps = caps + ",TEMPERATURE_2";
  if (CAPABILITIES_HUMIDITY_1 == 1)      caps = caps + ",HUMIDITY_1";
  if (CAPABILITIES_HUMIDITY_2 == 1)      caps = caps + ",HUMIDITY_2";
  if (CAPABILITIES_AIR_PRESSURE_1 == 1)  caps = caps + ",AIR_PRESSURE_1";
  if (CAPABILITIES_AIR_PRESSURE_2 == 1)  caps = caps + ",AIR_PRESSURE_2";
  if (CAPABILITIES_ALTITUDE_1 == 1)      caps = caps + ",ALTITUDE_1";
  if (CAPABILITIES_ALTITUDE_2 == 1)      caps = caps + ",ALTITUDE_2";
  if (CAPABILITIES_CO2_1 == 1)           caps = caps + ",CO2_1";
  if (CAPABILITIES_CO2_2 == 1)           caps = caps + ",CO2_2";
  if (CAPABILITIES_LIGHT_1 == 1)         caps = caps + ",LIGHT_1";
  if (CAPABILITIES_LIGHT_2 == 1)         caps = caps + ",LIGHT_2";
  if (CAPABILITIES_RAIN_1 == 1)          caps = caps + ",RAIN_1";
  if (CAPABILITIES_RAIN_2 == 1)          caps = caps + ",RAIN_2";

  // Build content
  msg = "{\"result\":{\"ROUTE\":\"/api/v1/capabilities\",\"status\":\"OK\", \"value\":\"";
  msg = msg + caps;
  msg = msg + "\"}}";

  // Send content to client
  server.send(200, "application/json", msg);
}



/**
   ROUTE - "/system/capabilities"
   @param void
   @return void
*/
void handleRoute_capabilities() {
  Serial.println("> WEB SERVER: Page Request 'Set Capabilities'.");
  
  if (checkCookieAuthed()) {

    // READ CAPABILITIES FROM EEPROM
    String msg;
    String t1,t2,h1,h2,ap1,ap2,al1,al2,c1,c2,l1,l2,r1,r2 = "";

    // Check capabilities
    if (CAPABILITIES_TEMERATURE_1 == 1)    t1  = " checked";
    if (CAPABILITIES_TEMERATURE_2 == 1)    t2  = " checked";
    if (CAPABILITIES_HUMIDITY_1 == 1)      h1  = " checked";
    if (CAPABILITIES_HUMIDITY_2 == 1)      h2  = " checked";
    if (CAPABILITIES_AIR_PRESSURE_1 == 1)  ap1 = " checked";
    if (CAPABILITIES_AIR_PRESSURE_2 == 1)  ap2 = " checked";
    if (CAPABILITIES_ALTITUDE_1 == 1)      al1 = " checked";
    if (CAPABILITIES_ALTITUDE_2 == 1)      al2 = " checked";
    if (CAPABILITIES_CO2_1 == 1)           c1  = " checked";
    if (CAPABILITIES_CO2_2 == 1)           c2  = " checked";
    if (CAPABILITIES_LIGHT_1 == 1)         l1  = " checked";
    if (CAPABILITIES_LIGHT_2 == 1)         l2  = " checked";
    if (CAPABILITIES_RAIN_1 == 1)          r1  = " checked";
    if (CAPABILITIES_RAIN_2 == 1)          r2  = " checked";

    // Build HTML
    msg = html_header;
    msg = msg + "<main>";
    msg = msg + "<h1>DASHBOARD<h1>";
    msg = msg + "<h3>API Endpoints<h3>";
    msg = msg + "  <p>";
    msg = msg + "    <h3>Set Capbilities</h3>";
    msg = msg + "  </p>";
    msg = msg + "  <p>";
    msg = msg + "   <form action=\"/system/capabilities/save\" method=\"POST\">";
    msg = msg + "    <input type='checkbox' id='CAPABILITIES_TEMERATURE_1' name='CAPABILITIES_TEMERATURE_1' value='1'" + t1 + ">";
    msg = msg + "    <label for='CAPABILITIES_TEMERATURE_1'>TEMERATURE #1</label><br>";
    
    msg = msg + "    <input type='checkbox' id='CAPABILITIES_TEMERATURE_2' name='CAPABILITIES_TEMERATURE_2' value='1'" + t2 + ">";
    msg = msg + "    <label for='CAPABILITIES_TEMERATURE_2'>TEMERATURE #2</label><br>";
    
    msg = msg + "    <input type='checkbox' id='CAPABILITIES_HUMIDITY_1' name='CAPABILITIES_HUMIDITY_1' value='1'" + h1 + ">";
    msg = msg + "    <label for='CAPABILITIES_HUMIDITY_1'>HUMIDITY #1</label><br>";
    
    msg = msg + "    <input type='checkbox' id='CAPABILITIES_HUMIDITY_2' name='CAPABILITIES_HUMIDITY_2' value='1'" + h2 + ">";
    msg = msg + "    <label for='CAPABILITIES_HUMIDITY_2'>HUMIDITY #2</label><br>";
    
    msg = msg + "    <input type='checkbox' id='CAPABILITIES_AIR_PRESSURE_1' name='CAPABILITIES_AIR_PRESSURE_1' value='1'" + ap1 + ">";
    msg = msg + "    <label for='CAPABILITIES_AIR_PRESSURE_1'>AIR PRESSURE #1</label><br>";
    
    msg = msg + "    <input type='checkbox' id='CAPABILITIES_AIR_PRESSURE_2' name='CAPABILITIES_AIR_PRESSURE_2' value='1'" + ap2 + ">";
    msg = msg + "    <label for='CAPABILITIES_AIR_PRESSURE_2'>AIR PRESSURE #2</label><br>";
    
    msg = msg + "    <input type='checkbox' id='CAPABILITIES_ALTITUDE_1' name='CAPABILITIES_ALTITUDE_1' value='1'" + al1 + ">";
    msg = msg + "    <label for='CAPABILITIES_ALTITUDE_1'>ALTITUDE #1</label><br>";
    
    msg = msg + "    <input type='checkbox' id='CAPABILITIES_ALTITUDE_2' name='CAPABILITIES_ALTITUDE_2' value='1'" + al2 + ">";
    msg = msg + "    <label for='CAPABILITIES_ALTITUDE_2'>ALTITUDE #2</label><br>";
    
    msg = msg + "    <input type='checkbox' id='CAPABILITIES_CO2_1' name='CAPABILITIES_CO2_1' value='1'" + c1 + ">";
    msg = msg + "    <label for='CAPABILITIES_CO2_1'>CO2 #1</label><br>";
    
    msg = msg + "    <input type='checkbox' id='CAPABILITIES_CO2_2' name='CAPABILITIES_CO2_2' value='1'" + c2 + "";
    msg = msg + "    <label for='CAPABILITIES_CO2_2'>CO2 #2</label><br>";
    
    msg = msg + "    <input type='checkbox' id='CAPABILITIES_LIGHT_1' name='CAPABILITIES_LIGHT_1' value='1'" + l1 + ">";
    msg = msg + "    <label for='CAPABILITIES_LIGHT_1'>LIGHT #1</label><br>";
    
    msg = msg + "    <input type='checkbox' id='CAPABILITIES_LIGHT_2' name='CAPABILITIES_LIGHT_2' value='1'" + l2 + ">";
    msg = msg + "    <label for='CAPABILITIES_LIGHT_2'>LIGHT #2</label><br>";
    
    msg = msg + "    <input type='checkbox' id='CAPABILITIES_RAIN_1' name='CAPABILITIES_RAIN_1' value='1'" + r1 + ">";
    msg = msg + "    <label for='CAPABILITIES_RAIN_1'>RAIN #1</label><br>";
    
    msg = msg + "    <input type='checkbox' id='CAPABILITIES_RAIN_2' name='CAPABILITIES_RAIN_2' value='1'" + r2 + ">";
    msg = msg + "    <label for='CAPABILITIES_RAIN_2'>RAIN #2</label><br>";
    msg = msg + "    <input type=\"submit\" value=\"Save!\"><br>";
    msg = msg + "   </form>";
    msg = msg + "  </p>";
    msg = msg + "  <p>";
    msg = msg + "    <a href='/dashboard'>[x] Back</a><br>";
    msg = msg + "  </p>";
    msg = msg + "    </main>";
    msg = msg + html_footer;
  
    // Send content to client
    setHeaders_NoCache();
    setHeaders_CrossOrigin();
    server.send(200, "text/html", msg);
  }
}


/**
   ROUTE - "/system/capabilities/save"
   @param void
   @return void
*/
void handleRoute_capabilities_Save() {
  Serial.println("> WEB SERVER: Page Request 'Set Capabilities - Saved'.");
  
  if (checkCookieAuthed()) {
    
    String inMODE = server.arg("mode");
    char msgResponse[255];
  
    if (server.arg("CAPABILITIES_TEMERATURE_1") == "1")
      write_EEPROM_LOCATION(EEPROM_CAPABILITIES_TEMERATURE_1, 1);

    if (server.arg("CAPABILITIES_TEMERATURE_2") == "1")
      write_EEPROM_LOCATION(EEPROM_CAPABILITIES_TEMERATURE_2, 1);

    if (server.arg("CAPABILITIES_HUMIDITY_1") == "1")
      write_EEPROM_LOCATION(EEPROM_CAPABILITIES_HUMIDITY_1, 1);

    if (server.arg("CAPABILITIES_HUMIDITY_2") == "1")
      write_EEPROM_LOCATION(EEPROM_CAPABILITIES_HUMIDITY_2, 1);

    if (server.arg("CAPABILITIES_AIR_PRESSURE_1") == "1")
      write_EEPROM_LOCATION(EEPROM_CAPABILITIES_AIR_PRESSURE_1, 1);

    if (server.arg("CAPABILITIES_AIR_PRESSURE_2") == "1")
      write_EEPROM_LOCATION(EEPROM_CAPABILITIES_AIR_PRESSURE_2, 1);

    if (server.arg("CAPABILITIES_ALTITUDE_1") == "1")
      write_EEPROM_LOCATION(EEPROM_CAPABILITIES_ALTITUDE_1, 1);

    if (server.arg("CAPABILITIES_ALTITUDE_2") == "1")
      write_EEPROM_LOCATION(EEPROM_CAPABILITIES_ALTITUDE_2, 1);

    if (server.arg("CAPABILITIES_CO2_1") == "1")
      write_EEPROM_LOCATION(EEPROM_CAPABILITIES_CO2_1, 1);

    if (server.arg("CAPABILITIES_CO2_2") == "1")
      write_EEPROM_LOCATION(EEPROM_CAPABILITIES_CO2_2, 1);

    if (server.arg("CAPABILITIES_LIGHT_1") == "1")
      write_EEPROM_LOCATION(EEPROM_CAPABILITIES_LIGHT_1, 1);
      
    if (server.arg("CAPABILITIES_LIGHT_2") == "1")
      write_EEPROM_LOCATION(EEPROM_CAPABILITIES_LIGHT_2, 1);

    if (server.arg("CAPABILITIES_RAIN_1") == "1")
      write_EEPROM_LOCATION(EEPROM_CAPABILITIES_RAIN_1, 1);

    if (server.arg("CAPABILITIES_RAIN_2") == "1")
      write_EEPROM_LOCATION(EEPROM_CAPABILITIES_RAIN_2, 1);

    // Read Capabiltiies from EEPROM into Global
    loadDeviceCapabilities();

    String msg;
    msg = html_header;
    msg = msg + "    <main>";
    msg = msg + "  <h1>Set Capabilities</h1>\r\n";
    msg = msg + "  <p>";
    msg = msg + "    Saved.";
    msg = msg + "  </p>";
    msg = msg + "  <p>";
    msg = msg + "    <a href='/dashboard'>[x] Back</a><br>";
    msg = msg + "  </p>";
    msg = msg + "    </main>";
    msg = msg + html_footer;
  
    // Send content to client
    setHeaders_NoCache();
    setHeaders_CrossOrigin();
    server.send(200, "text/html", msg);
  }
}


/**
   ROUTE - "/authentication/logout"
   @param void
   @return void
*/
void handleRoute_authentication_logout() {
  Serial.println("> WEB SERVER: Page Request 'Authentication - Logout'.");
  
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
  Serial.println("> WEB SERVER: Page Request 'New Wifi Details'.");
  
  if (checkCookieAuthed()) {
    
    String msg;
    msg =  html_header;
    msg += "<main>";
    msg += "  <h1>Set WIFI Details</h1>\r\n";
    msg += "  <p>";
    msg += "    <h3>Current Details:</h3> '"; msg += SSIDNAME; msg += "' / '"; msg += SSIDPASS; msg += "'.";
    msg += "  </p>";
    msg += "  <p>";
    msg += "    <form action=\"/system/set-wifi/save\" method=\"POST\">";
    msg += "      <h3>SSID Name</h3>";
    msg += "      <input type=\"text\" name=\"ssid\"><br>";
    msg += "      <h3>SSID Password</h3>";
    msg += "      <input type=\"password\" name=\"password\"><br><br>";
    msg += "      <input type=\"submit\" value=\"Save!\"><br>";
    msg += "    </form>";
    msg += "  </p>";
    msg += "  <p>";
    msg += "    <a href='/dashboard'>[x] Back</a><br>";
    msg += "  </p>";
    msg += "</main>";
    msg += html_footer;
  
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
  Serial.println("> WEB SERVER: Page Request 'New Wifi Details - Saved'.");
  
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
    strcpy(msg, " "); strcat(msg, html_header);
    strcat(msg, "    <main>");
    strcat(msg, "  <h1>Set WIFI Details</h1>\r\n");
    strcat(msg, "  <p>");
    strcat(msg, "    "); strcat(msg, msgResponse);
    strcat(msg, "  </p>");
    strcat(msg, "  <p>");
    strcat(msg, "    <a href='/dashboard'>[x] Back</a><br>");
    strcat(msg, "  </p>");
    strcat(msg, "    </main>");
    strcat(msg, html_footer);
  
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
  Serial.println("> WEB SERVER: Page Request 'New Hostname'.");
  
  if (checkCookieAuthed()) {
    
    char msg[2000];
    strcpy(msg, " "); strcat(msg, html_header);
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
    strcat(msg, html_footer);
  
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
  Serial.println("> WEB SERVER: Page Request 'New Hostname - Saved'.");
  
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
    strcpy(msg, " "); strcat(msg, html_header);
    strcat(msg, "    <main>");
    strcat(msg, "  <h1>Set Hostname</h1>\r\n");
    strcat(msg, "  <p>");
    strcat(msg, "    "); strcat(msg, msgResponse);
    strcat(msg, "  </p>");
    strcat(msg, "  <p>");
    strcat(msg, "    <a href='/dashboard'>[x] Back</a><br>");
    strcat(msg, "  </p>");
    strcat(msg, "    </main>");
    strcat(msg, html_footer);
  
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
  Serial.println("> WEB SERVER: Page Request 'Wipe Eeprom'.");
  
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
  Serial.println("> WEB SERVER: Page Request 'Authentication'.");
  
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
   ROUTE - "/api/v1/temperature/1/"
   Reports the Temperature from Temperature Sensor #1
   @param void
   @return void
*/
void handleRoute_api_temperature1() {
  Serial.println("> WEB SERVER: Page Request 'Temperature #1 as JSON'.");
  
  setHeaders_NoCache();
  setHeaders_CrossOrigin();

  // Define identity
  char msg[255];
  float r1 = 0.00;
  char rr1[10];
  
  // read sensor
  for (int i = 0; i < 5; i++) {
    r1 = bmp.readTemperature();
    if (r1 > -20 && r1 < 50)
      break;
  }
  r1 = roundf(r1 * 100) / 100;      // Round to 2 decimal places
  sprintf (rr1, "%f", r1);          // Convert to Char Array
  
  // Build content
  strcpy(msg, "{\"result\":{\"ROUTE\":\"/api/v1/temperature1\",\"status\":\"OK\", \"value\":\"");
  strcat(msg, rr1);
  strcat(msg, "\"}}");
  
  // Send content to client
  server.send(200, "application/json", msg);
}


/**
   ROUTE - "/api/v1/temperature/2/"
   Reports the Temperature from Temperature Sensor #2
   @param void
   @return void
*/
void handleRoute_api_temperature2() {
  Serial.println("> WEB SERVER: Page Request 'Temperature #2 as JSON'.");
  
  setHeaders_NoCache();
  setHeaders_CrossOrigin();

  // Define identity
  char msg[255];
  float r1 = 0.00;
  char rr1[10];
  
  // read sensor
  for (int i = 0; i < 5; i++) {
    r1 = dht1.readTemperature();
    if (r1 > -20 && r1 < 50)
      break;
  }
  r1 = roundf(r1 * 100) / 100;      // Round to 2 decimal places
  sprintf (rr1, "%f", r1);          // Convert to Char Array
  
  // Build content
  strcpy(msg, "{\"result\":{\"ROUTE\":\"/api/v1/temperature2\",\"status\":\"OK\", \"value\":\"");
  strcat(msg, rr1);
  strcat(msg, "\"}}");
  
  // Send content to client
  server.send(200, "application/json", msg);
}




/**
   ROUTE - "/api/v1/air-pressure/2/"
   Reports the Temperature from Air Pressure Sensor #1
   @param void
   @return void
*/
void handleRoute_api_air_pressure_1() {
  Serial.println("> WEB SERVER: Page Request 'Air Pressure #1 as JSON'.");
  
  setHeaders_NoCache();
  setHeaders_CrossOrigin();

  // Define identity
  String msg;
  float r1, r2 = 0.00;
  char rr1[10];
  double T,P,p0,a;

  // read sensor
  r1 = bmp.readPressure();
  r2 = bmp.readSealevelPressure(ALTITUDE);
  r1 = r1 / 100;
  r2 = r2 / 100;

  // Build content
  msg =  "{\"result\":{\"ROUTE\":\"/api/v1/air-pressure/1\",\"status\":\"OK\", \"value\":\"";
  msg += r1;
  msg += ",";
  msg += r2;
  msg += "\"}}";
  // Send content to client
  server.send(200, "application/json", msg);
}




/**
   ROUTE - "/api/v1/humidity/1/"
   Reports the Temperature from Humidity Sensor #1
   @param void
   @return void
*/
void handleRoute_api_humidity1() {
  Serial.println("> WEB SERVER: Page Request 'Humidity #1 as JSON'.");
  
  setHeaders_NoCache();
  setHeaders_CrossOrigin();

  // Define identity
  char msg[255];
  float r1 = 0.00;
  char rr1[10];
  
  // read sensor
  for (int i = 0; i < 5; i++) {
    r1 = dht1.readHumidity(); // Gets the values of the humidity
    if (r1 > -20 && r1 < 50)
      break;
  }
  r1 = roundf(r1 * 100) / 100;      // Round to 2 decimal places
  sprintf (rr1, "%f", r1);          // Convert to Char Array
  
  // Build content
  strcpy(msg, "{\"result\":{\"ROUTE\":\"/api/v1/humidity/1/\",\"status\":\"OK\", \"value\":\"");
  strcat(msg, rr1);
  strcat(msg, "\"}}");
  
  // Send content to client
  server.send(200, "application/json", msg);
}


/**
   ROUTE - "/api/v1/humidity/2/"
   Reports the Temperature from Temperature Sensor #2
   @param void
   @return void
*/
void handleRoute_api_humidity2() {
  Serial.println("> WEB SERVER: Page Request 'Humidity #2 as JSON'.");
  
  setHeaders_NoCache();
  setHeaders_CrossOrigin();

  // Define identity
  char msg[255];
  float r1 = 0.00;
  char rr1[10];
  
  // read sensor
  for (int i = 0; i < 5; i++) {
    r1 = dht2.readHumidity(); // Gets the values of the humidity
    if (r1 > -20 && r1 < 50)
      break;
  }
  r1 = roundf(r1 * 100) / 100;      // Round to 2 decimal places
  sprintf (rr1, "%f", r1);          // Convert to Char Array
  
  // Build content
  strcpy(msg, "{\"result\":{\"ROUTE\":\"/api/v1/humidity/2/\",\"status\":\"OK\", \"value\":\"");
  strcat(msg, rr1);
  strcat(msg, "\"}}");
  
  // Send content to client
  server.send(200, "application/json", msg);
}


/**
   ROUTE - "/api/v1/co2/1/"
   Reports the Temperature from CO2 Sensor #1
   @param void
   @return void
*/
void handleRoute_api_co2_1() {
  Serial.println("> WEB SERVER: Page Request 'CO2 #1 as JSON'.");
  
  setHeaders_NoCache();
  setHeaders_CrossOrigin();

  // Define identity
  String msg;
  float r1 = 0.00;
  char rr1[10];

  r1 = analogRead(CO2Pin_1);
  
  // Build content
  msg =  "{\"result\":{\"ROUTE\":\"/api/v1/co2/1/\",\"status\":\"OK\", \"value\":\"";
  msg += r1;
  msg += "\"}}";

  // Send content to client
  server.send(200, "application/json", msg);
}



/** Ultility Functions **/
//////////////////////////

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
  char *key;
  char *payload;
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
  
  strcat(hash, key);
  strcat(hash, "-");
  strcat(hash, payload);

  // Generate HASH String
  //mbedtls_md_context_t ctx;
  //mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

  //const size_t payloadLength = strlen(payload);
  //const size_t keyLength = strlen(key);

  //mbedtls_md_init(&ctx);
  //mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
  //mbedtls_md_hmac_starts(&ctx, (const unsigned char *) key, keyLength);
  //mbedtls_md_hmac_update(&ctx, (const unsigned char *) payload, payloadLength);
  //mbedtls_md_hmac_finish(&ctx, hmacResult);
  //mbedtls_md_free(&ctx);

  //for (int i = 0; i < sizeof(hmacResult); i++) {
  //  char str[3];
  //  sprintf(str, "%02x", (int)hmacResult[i]);
  //  hashf = hashf + str;
  //}
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
 * Read single location from EEPROM
 * @param void
 * @return void
 */
int read_EEPROM_LOCATION (int EEPROM_LOC) {
  int r = 0;
  r = EEPROM.read(EEPROM_LOC);
  return r;
}


/**
 * Read single location from EEPROM
 * @param void
 * @return void
 */
void write_EEPROM_LOCATION (int EEPROM_LOC, int LOC_VAL) {
  EEPROM.write(EEPROM_LOC, LOC_VAL);
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
  EEPROM.commit();
  return true;
}



/**
 * Loads the saved device capabilities into Memory
 * such as if it has Temperature, Humidity, etc sensors
 * @param void
 * @return void
 */
void loadDeviceCapabilities () {
  CAPABILITIES_TEMERATURE_1   = read_EEPROM_LOCATION(EEPROM_CAPABILITIES_TEMERATURE_1);
  CAPABILITIES_TEMERATURE_2   = read_EEPROM_LOCATION(EEPROM_CAPABILITIES_TEMERATURE_2);
  CAPABILITIES_HUMIDITY_1     = read_EEPROM_LOCATION(EEPROM_CAPABILITIES_HUMIDITY_1);
  CAPABILITIES_HUMIDITY_2     = read_EEPROM_LOCATION(EEPROM_CAPABILITIES_HUMIDITY_2);
  CAPABILITIES_AIR_PRESSURE_1 = read_EEPROM_LOCATION(EEPROM_CAPABILITIES_AIR_PRESSURE_1);
  CAPABILITIES_AIR_PRESSURE_2 = read_EEPROM_LOCATION(EEPROM_CAPABILITIES_AIR_PRESSURE_2);
  CAPABILITIES_ALTITUDE_1     = read_EEPROM_LOCATION(EEPROM_CAPABILITIES_ALTITUDE_1);
  CAPABILITIES_ALTITUDE_2     = read_EEPROM_LOCATION(EEPROM_CAPABILITIES_ALTITUDE_2);
  CAPABILITIES_CO2_1          = read_EEPROM_LOCATION(EEPROM_CAPABILITIES_CO2_1);
  CAPABILITIES_CO2_2          = read_EEPROM_LOCATION(EEPROM_CAPABILITIES_CO2_2);
  CAPABILITIES_LIGHT_1        = read_EEPROM_LOCATION(EEPROM_CAPABILITIES_LIGHT_1);
  CAPABILITIES_LIGHT_2        = read_EEPROM_LOCATION(EEPROM_CAPABILITIES_LIGHT_2);
  CAPABILITIES_RAIN_1         = read_EEPROM_LOCATION(EEPROM_CAPABILITIES_RAIN_1);
  CAPABILITIES_RAIN_2         = read_EEPROM_LOCATION(EEPROM_CAPABILITIES_RAIN_2);
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
  char chars[100] = {" abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@£$%^&*-_=+~#/?>.<,|;:'"};
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
  char chars[100] = {" abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@£$%^&*-_=+~#/?>.<,|;:'"};
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



