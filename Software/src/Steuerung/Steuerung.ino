#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#include "DebugOut.h"
#include "Settigs.h"
#include "WifiConnect.h"

Settings data;

void setup() {
	data.init();
}

void loop() {
}