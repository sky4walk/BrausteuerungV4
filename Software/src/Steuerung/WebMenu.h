#ifndef __WEBMENU__
#define __WEBMENU__

#include <ESP8266WebServer.h>
#include "settings.h"

class WebMenu {
  public:
    WebMenu(Settings& data)
      :	mServer(80), mData(data) {
    }
    void init() {
      mServer.begin();
      mServer.on("/", std::bind(&WebMenu::setUpWifi, this));
    }
    void polling() {
      mServer.handleClient();
    }
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
      webpage += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32>";
      webpage += "<input name='pass' length=64><input type='submit'></form>";
      //webpage += "<form action='http://" + WiFi.localIP().toString() + "' method='POST'>";
      //webpage += "SSID<input type='text' name='ssis_input'><BR>";
      //webpage += "Password:<input type='text' name='password_input'>&nbsp;<input type='submit' value='Set'>";
      //webpage += "</form>";
      webpage += "</body>";
      webpage += "</html>";

      mServer.send(200, "text/html", webpage);
      
      String ssid = mServer.arg("ssid");
      String pass = mServer.arg("pass");
      mData.setWifi(ssid,pass);
      webpage = "{\"Success\":\"saved to new wifi data\"}";
    }
  private:
    ESP8266WebServer mServer;
    Settings& mData;
};

#endif
