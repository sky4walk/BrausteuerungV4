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
      /*
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
            return true;
          }
          c++;
        }
      }
      */
      return false;
    }

    void setupAP() {
      IPAddress myIP(192, 168, 4, 1);
      IPAddress subnet(255, 255, 255, 0);
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF); 
      WiFi.mode(WIFI_AP);
      //WiFi.mode(WIFI_AP_STA);
      WiFi.softAPConfig(myIP, myIP, subnet);
      WiFi.softAP(APSSID);       
      delay(1000);      
      IPAddress myIPget = WiFi.softAPIP(); 
      DebugOut::debug_out("AccessPoint");
      DebugOut::debug_out(myIPget);   
    }
  private:
    Settings& mData;
};

#endif
