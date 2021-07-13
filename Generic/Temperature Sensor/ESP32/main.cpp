#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <WiFi.h>
#include <WebServer.h>

// Constants
#define HOSTNAME "X-SENSOR"
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
  WiFi.begin(SSIDNAME, SSIDPASS);
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
  server.on("/",                HTTP_GET,  handleRoute_root);
  server.on("/dashboard/",      HTTP_POST, handleRoute_dashboard);
  server.on("/api/temperature", HTTP_GET,  handleRoute_temperature);
  server.on("/api/pressure",    HTTP_GET,  handleRoute_pressure);
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
  setHeaders_NoCache();
  setHeaders_CrossOrigin();
  
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
  strcat(msg, "      <h1>");  strcat(msg, HOSTNAME);  strcat(msg, " - LOGIN</h1>\r\n");
  strcat(msg, "      <p>");
  strcat(msg, "        <form action=\"/dashboard/\" method=\"POST\">");
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
  
  server.send(200, "text/html", msg);
}


/**
 * ROUTE - "/dashboard/"
 */
void handleRoute_dashboard() {
  setHeaders_NoCache();
  setHeaders_CrossOrigin();

  // Check login credentials
  String username = server.arg("username");
  String password = server.arg("password");
  if (username != APIUSER || password != APIPASS) {
    server.sendHeader(F("Location"), F("/"));
    server.send(401, "text/html", "Not Authorised.");
  }

  // Else Carry on...
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
  strcat(msg, "        <a href='/'>[X] Logout</a><br>");
  strcat(msg, "      </p>");
  strcat(msg, "    </main>");
  strcat(msg, "  </body>");
  strcat(msg, "</html>");
  
  server.send(200, "text/html", msg);
}



/**
 * ROUTE - "/api/temperature"
 */
void handleRoute_temperature() {
  setHeaders_NoCache();
  setHeaders_CrossOrigin();
  
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
 * ROUTE - "/api/pressure"
 */
void handleRoute_pressure() {
  setHeaders_NoCache();
  setHeaders_CrossOrigin();
  
  float r = 0.00;
  char r1[10];
  int count = 0;
  char count1[2];
  char msg[1000];
  
  // read sensor
  for (int i=0; i<40; i++) {
    count = i;
    r = bmp.readPressure();
    if (r > 850 && r < 120000)
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


void setHeaders_NoCache(){
  server.sendHeader(F("Expires"),       F("-1"));
  server.sendHeader(F("Pragma"),        F("no-cache"));
  server.sendHeader(F("cache-control"), F("no-cache, no-store"));
};

void setHeaders_CrossOrigin(){
  server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
  server.sendHeader(F("Access-Control-Max-Age"), F("600"));
  server.sendHeader(F("Access-Control-Allow-Methods"), F("PUT,POST,GET,OPTIONS"));
  server.sendHeader(F("Access-Control-Allow-Headers"), F("*"));
};
