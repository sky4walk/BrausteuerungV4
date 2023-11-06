// brausteuerung@AndreBetz.de
#ifndef __WIFICONFIGURATION__
#define __WIFICONFIGURATION__

#if defined(ESP8266)
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#else
#include <WiFi.h>
#endif

#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <ESP8266mDNS.h>
#include "settings.h"
#include "DbgConsole.h"

#define PORTALTIMEOUT 300
#define TKIDSIZE 40
#define APNAME "Zwieselbrau.de"
#define HOSTNAME "Zwieselbrau"

class WifiConfiguration {
  public:
    WifiConfiguration(Settings& set) :
       server(80),
       wifiManager(&server,&dns),
       mSettings(set) {
    }
    void begin(){
//      wifiManager.setConfigPortalTimeout(PORTALTIMEOUT);  
//      wifiManager.setBreakAfterConfig(true);
      AsyncWiFiManagerParameter custom_password  ("wbpasswd", "WebPassWD", mSettings.getWebPassWd().c_str(), TKIDSIZE);      
      wifiManager.addParameter(&custom_password);     
      //wifiManager.setTimeout(PORTALTIMEOUT);      
      wifiManager.setTryConnectDuringConfigPortal(false);      
    }
    bool config()
    {    
      CONSOLELN(F("started Portal"));
      wifiManager.startConfigPortal(APNAME);
      CONSOLELN(F("set wifi config"));
      CONSOLELN(wifiManager.getConfigPortalSSID());     
      return true;
    }
    bool connect() {
      bool ret = wifiManager.autoConnect(APNAME);
      if (!ret) {
        CONSOLELN("Error connect to Wifi!");
        return false;
      }
      if (!MDNS.begin(HOSTNAME)) {
        CONSOLELN("Error setting up MDNS responder!");
        while(1) {
          delay(1000);
        }
      }
      return true;
    }
    AsyncWebServer& getWebserver() {
      return server;
    }
  private:
    AsyncWebServer server;
    DNSServer dns;
    AsyncWiFiManager wifiManager;
    Settings& mSettings;
};


#endif
