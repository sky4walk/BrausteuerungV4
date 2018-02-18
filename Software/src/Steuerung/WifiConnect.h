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
        //WiFi.disconnect();
        //WiFi.mode(WIFI_STA);
        WiFi.mode(WIFI_AP_STA);
        WiFi.begin(ssid.c_str(), pass.c_str());
        DebugOut::debug_out(ssid+" "+pass);
        while ( c < 20 ) {          
          DebugOut::debug_out(c);
          delay(500);
          ESP.wdtFeed();
          yield();
          if (WiFi.status() == WL_CONNECTED) {
            DebugOut::debug_out(WiFi.localIP());
            return true;
          }
          c++;
        }
      }
      return false;
    }

    void setupAP() {
      IPAddress myIP(192, 168, 4, 1);
      IPAddress subnet(255, 255, 255, 0);
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF); 
      WiFi.mode(WIFI_AP);
      WiFi.softAPConfig(myIP, myIP, subnet);
      WiFi.softAP(APSSID);       
      delay(1000);      
      IPAddress myIPget = WiFi.softAPIP(); 
      DebugOut::debug_out(myIPget);
    }
  private:
    Settings& mData;
};

#endif
