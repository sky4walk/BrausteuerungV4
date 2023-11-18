// brausteuerung@AndreBetz.de
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SteuerungWebServer.h"
#include "DbgConsole.h"

static AsyncWebServer mServer(80);

SteuerungWebServer::SteuerungWebServer(Settings& set) :
  mSettings(set) {
}

void SteuerungWebServer::begin() {
  SPIFFS.begin();
  mServer.begin();

  ///////////////////////////////////////////////////////////////////////////
  // Root
  ///////////////////////////////////////////////////////////////////////////
  mServer.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", F("Hallo World"));
  });
}