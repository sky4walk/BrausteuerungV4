#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#define DEBUG_OUT
#define WEBNAME "mikroSikaru.de"

#include "DebugOut.h"
#include "Settings.h"
#include "WifiConnect.h"
#include "WebMenu.h"

Settings data;
WifiConnect netzwerk(data);
WebMenu menu(data);

void setup() {
	data.init();
  netzwerk.init();
  
  Serial.begin(115200);

  if ( !netzwerk.testWifi() )
  {
    netzwerk.setupAP();
  }

  menu.init();
  
}

void loop() {
  menu.polling();
}
