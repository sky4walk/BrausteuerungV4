#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

ESP8266WebServer server(80);

const char* APssid = "mikroSikaru.de";
boolean _debug = true;

void setUpWifi() {

  int n = WiFi.scanNetworks();

  String webpage = "";
  webpage =  "<html><head><title>Wifi Setup</title></head>";
  webpage += "<body>";
  webpage += "<h1><br>Access points</h1>";
  for (int i = 0; i < n; ++i)
  {
    webpage += WiFi.SSID(i);
    webpage += " (";
    webpage += WiFi.RSSI(i);
    webpage += ")";
    webpage += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    webpage += "<BR>";
  }
  webpage += "<form action='http://" + WiFi.localIP().toString() + "' method='POST'>";
  webpage += "SSID<input type='text' name='ssis_input'><BR>";
  webpage += "Password:<input type='text' name='password_input'>&nbsp;<input type='submit' value='Set'>";
  webpage += "</form>";
  webpage += "</body>";
  webpage += "</html>";

  server.send(200, "text/html", webpage);
  if (server.args() > 0 ) { 
    for ( uint8_t i = 0; i < server.args(); i++ ) {
      Serial.print(server.argName(i)); // Display the argument
      Argument_Name = server.argName(i);
      if (server.argName(i) == "name_input") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        Clients_Response1 = server.arg(i);
      }
      if (server.argName(i) == "address_input") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        Clients_Response2 = server.arg(i);
      }
    }
  }

  boolean testWifi() {
    String esid = "";
    String epass = "";
    for (int i = 0; i < 32; ++i) {
      esid += char(EEPROM.read(i));
    }
    for (int i = 32; i < 96; ++i) {
      epass += char(EEPROM.read(i));
    }
    if ( esid.length() > 1 ) {
      int c = 0;
      WiFi.begin(esid.c_str(), epass.c_str());
      while ( c < 20 ) {
        delay(500);
        if (WiFi.status() == WL_CONNECTED)
          return true;
        c++;
      }
    }
    return false;
  }

  void setupAP() {
    //WiFi.disconnect();

    IPAddress myIP(10, 0, 0, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.mode(WIFI_AP_STA);
    WiFi.config(myIP, myIP, subnet);
    WiFi.softAP(APssid);
    delay(2000);
  }

  void setup() {

    delay(1000);

    Serial.begin(115200);

    if ( !testWifi() )
    {
      setupAP();
      server.on("/", setUpWifi);
    }

    server.begin();

  }

  void loop() {

    server.handleClient();

  }
