#ifndef __WIFICONNECT__
#define __WIFICONNECT__

#include <ESP8266mDNS.h>
#include "DebugOut.h"
#include "Settings.h"

class WifiConnect {
  public:
    WifiConnect(Settings& data)
      : mData(data) {
    }
    void init() {
    }

    bool testWifi() {
      String ssid = mData.getSSID();
      String pass = mData.getPASS();
      if ( ssid.length() > 1 ) {
        int c = 0;
        WiFi.begin(ssid.c_str(), pass.c_str());
        while ( c < 20 ) {
          delay(500);
          if (WiFi.status() == WL_CONNECTED) {
            DebugOut::debug_out("Connected");
            DebugOut::debug_out(WiFi.localIP());
            if ( MDNS.begin(WEBNAME) ) {
              DebugOut::debug_out("DNS");
            }
            return true;
          }
          c++;
        }
      }
      return false;
    }

    void setupAP() {
      //WiFi.disconnect();
      IPAddress myIP(192, 168, 4, 1);
      IPAddress subnet(255, 255, 255, 0);
      WiFi.mode(WIFI_AP_STA);
      WiFi.config(myIP, myIP, subnet);
      WiFi.softAP(WEBNAME);

      if (WiFi.waitForConnectResult() == WL_CONNECTED) {
        MDNS.begin(WEBNAME);
        MDNS.addService("http", "tcp", 80);
      }
      DebugOut::debug_out("AccessPoint");
      DebugOut::debug_out(WiFi.localIP());
    }
  private:
    Settings& mData;
};

#endif
