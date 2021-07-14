#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <WiFi.h>
#include <WebServer.h>
#include "mbedtls/md.h"
#include <time.h>
#include <stdlib.h>

// Constants
#define HOSTNAME "X-SENSOR-TEMP1"
#define SSIDNAME ""
#define SSIDPASS ""
#define APIUSER "x-smart"
#define APIPASS "fdTE%G54m2dY!g78"

// Variables
String SESSION_COOKIE_KEY = "none";
String header;

// Instantiate Objects / Devices
WebServer server(80);
Adafruit_BMP085 bmp;


/**
 * Setup method for ESP32
 * @param void
 * @return void
 */
void setup() {

  // SERIAL - Setup serial
  Serial.begin(9600);
  Serial.println("Serial Started.");
  delay(1000);
  
  // WIFI - Setup wifi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); delay(100);
  WiFi.setHostname(HOSTNAME);
  WiFi.begin(SSIDNAME, SSIDPASS);
  WiFi.setHostname(HOSTNAME);
  
  // WIFI - Wait until connected
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(100);
  }

  // WIFI - Display connection details
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
  
  // WEB SERVER - Define Routes
  server.on("/",                HTTP_GET,  handleRoute_root);
  server.on("/authentication",  HTTP_POST, handleRoute_authentication);
  server.on("/dashboard/",      HTTP_GET,  handleRoute_dashboard);
  server.on("/api/temperature", HTTP_GET,  handleRoute_temperature);
  server.on("/api/pressure",    HTTP_GET,  handleRoute_pressure);

  // WEB SERVER - Define which request headers to collect
  const char *headers[] = {"Host","Referer","Cookie"};
  size_t headersCount = sizeof(headers)/sizeof(char*);
  server.collectHeaders(headers, headersCount);

  // WEB SERVER - Begin!
  server.begin();
  Serial.println("API Server Started.");
  delay(1000);

  // BMP180 - Test component
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085/BMP180 sensor, check wiring!");
    while (1) {}
  }
}


/**
 * Main loop method
 * @param void
 * @return void
 */
void loop() {
  server.handleClient();
}


/**
 * ROUTE - "/"
 * @param void
 * @return void
 */
void handleRoute_root() {
  char msg[2000];
  strcpy(msg, "<html>");
  strcat(msg, "  <head>");
  strcat(msg, "    <title>");  strcat(msg, HOSTNAME);  strcat(msg, " - ESP32 SENSOR</title>\r\n");
  strcat(msg, "    <style type=\"text/css\">\r\n");
  strcat(msg, "      body  { background-color:#555; font-size:16px; color:#444; font-family: Serif; box-sizing: border-box; }\r\n");
  strcat(msg, "      main  { margin:5% auto; width:100%; max-width:360px; padding:30px 60px; background-color:#eee; border-radius:10px; box-shadow:0 0 10px #333; }\r\n");
  strcat(msg, "      h1    { font-size:24px; font-weight:bold;   }\r\n");
  strcat(msg, "      p     { font-size:16px; font-weight:normal; }\r\n");
  strcat(msg, "      a     { color:#99dd00; } a:visited { color:#99dd00; }\r\n");
  strcat(msg, "      input { border:2px solid #bbb; color:#444; border-radius:5px; padding:5px; }\r\n");
  strcat(msg, "    </style>\r\n");
  strcat(msg, "  </head>");
  strcat(msg, "  <body>");
  strcat(msg, "    <main>");
  strcat(msg, "      <h1>");  strcat(msg, HOSTNAME);  strcat(msg, " - LOGIN</h1>\r\n");
  strcat(msg, "      <p>");
  strcat(msg, "        <form action=\"/authentication\" method=\"POST\">");
  strcat(msg, "          <p>Username</p>");
  strcat(msg, "          <input type=\"text\" name=\"username\"><br>");
  strcat(msg, "          <p>Password</p>");
  strcat(msg, "          <input type=\"password\" name=\"password\"><br><br>");
  strcat(msg, "          <input type=\"submit\" value=\"Login!\"><br>");
  strcat(msg, "        </form>");
  strcat(msg, "      </p>");
  strcat(msg, "    </main>");
  strcat(msg, "  </body>");
  strcat(msg, "</html>");

  // Send content to client
  setHeaders_NoCache();
  setHeaders_CrossOrigin();
  server.send(200, "text/html", msg);
}


/**
 * ROUTE - "/authentication"
 * @param void
 * @return void
 */
void handleRoute_authentication() {
  String username = server.arg("username");
  String password = server.arg("password");
  
  // Check credentials are correct, or has cookie, then login
  if ((username == APIUSER && password == APIPASS) || checkCookieAuthedBool()) {
    setSessionCookie();
    server.sendHeader(F("Location"), F("/dashboard/"));
    server.send(302, "text/html", "Authorised.");
  
  // Else redirect back to login page
  } else {
    destroySessionCookie();
    server.sendHeader(F("Location"), F("/?status=no"));
    server.send(302, "text/html", "Not Authorised.");
  }
}


/**
 * ROUTE - "/dashboard/"
 * @param void
 * @return void
 */
void handleRoute_dashboard() {
  checkCookieAuthed();

  // Else, build content for page
  char msg[2000];
  strcpy(msg, "<html>");
  strcat(msg, "  <head>");
  strcat(msg, "    <title>");  strcat(msg, HOSTNAME);  strcat(msg, " - ESP32 SENSOR</title>\r\n");
  strcat(msg, "    <style type=\"text/css\">\r\n");
  strcat(msg, "      body  { background-color:#555; font-size:16px; color:#444; font-family: Serif; box-sizing: border-box; }\r\n");
  strcat(msg, "      main  { margin:5% auto; width:100%; max-width:360px; padding:30px 60px; background-color:#eee; box-shadow:0 0 10px #333; }\r\n");
  strcat(msg, "      h1    { font-size:24px; font-weight:bold;   }\r\n");
  strcat(msg, "      p     { font-size:16px; font-weight:normal; }\r\n");
  strcat(msg, "      a     { color:#99dd00; } a:visited { color:#99dd00; }\r\n");
  strcat(msg, "      input { border:2px solid #bbb; color:#444; border-radius:5px; padding:5px; }\r\n");
  strcat(msg, "    </style>\r\n");
  strcat(msg, "  </head>");
  strcat(msg, "  <body>");
  strcat(msg, "    <main>");
  strcat(msg, "      <h1>DASHBOARD<h1>");
  strcat(msg, "      <p>");
  strcat(msg, "        <a href='/api/temperature'>/api/temperature</a><br>");
  strcat(msg, "        <a href='/api/pressure'>/api/pressure</a><br>");
  strcat(msg, "      </p>");
  strcat(msg, "      <p>");
  strcat(msg, "        <a href='/'>[x] Logout</a><br>");
  strcat(msg, "      </p>");
  strcat(msg, "    </main>");
  strcat(msg, "  </body>");
  strcat(msg, "</html>");

  // Send content to client
  setHeaders_NoCache();
  setHeaders_CrossOrigin();
  server.send(200, "text/html", msg);
}


/**
 * ROUTE - "/api/temperature"
 * @param void
 * @return void
 */
void handleRoute_temperature() {
  checkCookieAuthed();

  // Define variables
  float r = 0.00;
  char r1[10];
  char msg[1000];
  
  // Read sensor
  r = readSensor("temperature");
  sprintf (r1, "%f", r);

  // Build content
  strcpy(msg, "{result:{\"ROUTE\":\"temperature\",\"status\":\"OK\", \"value\":\"");
  strcat(msg, r1);
  strcat(msg, "\"}}");

  // Send content to client
  setHeaders_NoCache();
  setHeaders_CrossOrigin();
  server.send(200, "application/json", msg);
}


/**
 * ROUTE - "/api/pressure"
 * @param void
 * @return void
 */
void handleRoute_pressure() {
  checkCookieAuthed();

  // Define variables
  float r = 0.00;
  char r1[10];
  char msg[1000];
  
  // Read sensor
  r = readSensor("pressure");
  sprintf (r1, "%f", r);

  // Build content
  strcpy(msg, "{result:{\"ROUTE\":\"pressure\",\"status\":\"OK\", \"value\":\"");
  strcat(msg, r1);
  strcat(msg, "\"}}");

  // Send content to client
  setHeaders_NoCache();
  setHeaders_CrossOrigin();
  server.send(200, "application/json", msg);
}


/**
 * Reas the sensor and return the value
 * @param String sensor
 * @param float r
 * @return char*
 */
float readSensor (String sensor) {
  float r = 0.00;
  char r1[10];
  
  // read sensor
  for (int i=0; i<40; i++) {

    // TEMPERATURE SENSOR
    if (sensor == "temperature") {
      r = bmp.readTemperature();
      Serial.println(r);
      if (r > -20 && r < 40)
        break;
        
    // PRESSURE SENSOR
    } else if (sensor == "pressure") {
      r = bmp.readPressure();
      Serial.println(r);
      if (r > 850 && r < 120000)
        break;
    }
    delay(50);
  }
  return r;
}


/**
 * Set the NoCaching headers for the browser
 * @param void
 * @return void
 */
void setHeaders_NoCache(){
  server.sendHeader(F("Expires"),       F("-1"));
  server.sendHeader(F("Pragma"),        F("no-cache"));
  server.sendHeader(F("cache-control"), F("no-cache, no-store"));
};


/**
 * Set the CORS headers for the browser
 * @param void
 * @return void
 */
void setHeaders_CrossOrigin(){
  server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
  server.sendHeader(F("Access-Control-Max-Age"), F("600"));
  server.sendHeader(F("Access-Control-Allow-Methods"), F("POST,GET"));
  server.sendHeader(F("Access-Control-Allow-Headers"), F("*"));
};


/**
 * Creates a new SESSION
 * @param void
 * @return void
 */
void setSessionCookie () {
  char *key = "";
  char *payload = "";
  byte hmacResult[32];
  char hash[255] = "";
  int  r1, r2 = 0;
  char r3[20], r4[20] = "";
  
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
  
  for(int i= 0; i< sizeof(hmacResult); i++){
      char str[3];
      sprintf(str, "%02x", (int)hmacResult[i]);
      strcat(hash, str);
  }

  // Build cookie string
  char cookie[265] = "X-SESSION=";
  strcat(cookie, hash);
  
  // Set cookie
  server.sendHeader("Set-Cookie", cookie);
  SESSION_COOKIE_KEY = cookie;
}


/**
 * Destroys the SESSION
 * @param void
 * @return void
 */
void destroySessionCookie () {
  server.sendHeader("Set-Cookie", "X-SESSION=none");
}


/**
 * Checks if authed with cookie, then redirects to login if not.
 * @param void
 * @return void
 */
void checkCookieAuthed () {
  
  Serial.print("COOKIE: |");
  Serial.print(SESSION_COOKIE_KEY);
  Serial.println("|");
  String cv = server.header("Cookie");
  Serial.print("COOKIE: |");
  Serial.print(cv);
  Serial.println("|");
  if (server.hasHeader("Host")) {
    Serial.print("COOKIE: |");
    Serial.print("YES");
    Serial.println("|");
  }
  
  String cookie_value = "09v2n548n243";
  if (server.hasHeader("Cookie") && SESSION_COOKIE_KEY != "")
    cookie_value = server.header("Cookie");
  if (cookie_value != SESSION_COOKIE_KEY) {
    server.sendHeader(F("Location"), F("/?status=no"));
    server.send(302, "text/html", "Not Authorised.");
  }
}


/**
 * Checks if authed with cookie, then returns true/false.
 * @param void
 * @return void
 */
bool checkCookieAuthedBool () {
  String cookie_value = "09v2n548n243";
  if (server.hasHeader("Cookie") && SESSION_COOKIE_KEY != "")
    cookie_value = server.header("Cookie");
  if (cookie_value == SESSION_COOKIE_KEY)
    return true;
  return false;
}
