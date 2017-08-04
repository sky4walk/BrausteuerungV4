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
      mServer.on("/",   std::bind(&WebMenu::startMenu, this));
      mServer.on("/sw", std::bind(&WebMenu::setUpWifi, this));
      mServer.on("/ss", std::bind(&WebMenu::setUpWifi, this));
      mServer.on("/re", std::bind(&WebMenu::setUpWifi, this));
      mServer.on("/br", std::bind(&WebMenu::setUpWifi, this));
      mServer.on("/fu", std::bind(&WebMenu::setUpWifi, this));
      mServer.on("/fu", HTTP_POST, [this]() {
          mServer.sendHeader("Connection", "close");
          mServer.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
          ESP.restart();
        }, std::bind(&WebMenu::FWUpdate, this)
      );
      mServer.onNotFound([this]() {
        mServer.send(404, "text/plain", "404: Not found");
      });
      mServer.begin();
    }
    void polling() {
      mServer.handleClient();
    }
  private:
    void setUpWifi() {
      DebugOut::debug_out("setUpWifi");
      int n = WiFi.scanNetworks();

      String webpage = "";
      webpage =  "<html><head><title>Wifi Setup</title></head>";
      webpage += "<body>";
      webpage += "<h1>Access points</h1>";
      for (int i = 0; i < n; ++i)
      {
        webpage += WiFi.SSID(i);
        webpage += " (";
        webpage += WiFi.RSSI(i);
        webpage += ")";
        webpage += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
        webpage += "<BR>";
      }
      webpage += "</p><form method='get' action='setting'>";
      webpage += "<label>SSID: </label>";
      webpage += "<input name='ssid' length=32>";
      webpage += "<label>PassWord: </label>";
      webpage += "<input name='pass' length=64>";
      webpage += "<input type='submit'></form>";
      webpage += "</body>";
      webpage += "</html>";

      mServer.send(200, "text/html", webpage);

      String ssid = mServer.arg("ssid");
      String pass = mServer.arg("pass");
      DebugOut::debug_out("setUpWifi " + ssid + " " + pass);
      mData.setWifi(ssid, pass);
      startMenu();
    }

    void startMenu() {
      DebugOut::debug_out("startMenu");
      String webpage = "";
      webpage =  "<html><head><title>Wifi Setup</title></head>";
      webpage += "<h1>Main Menu</h1>";
      webpage += "<body>";
      webpage += "<form action='http://" + WiFi.localIP().toString() + "/sw' method='POST'>";
      webpage += "<button>Setup Wifi</button></form><br>";
      webpage += "<form action='http://" + WiFi.localIP().toString() + "/ss' method='POST'>";
      webpage += "<button>Setup Switch</button></form><br>";
      webpage += "<form action='http://" + WiFi.localIP().toString() + "/re' method='POST'>";
      webpage += "<button>Recipe</button></form><br>";
      webpage += "<form action='http://" + WiFi.localIP().toString() + "/br' method='POST'>";
      webpage += "<button>Brew</button></form><br>";
      webpage += "<form action='http://" + WiFi.localIP().toString() + "/fu' method='POST'>";
      webpage += "<button>FW update</button></form><br>";
      webpage += "<form action='http://";
      webpage +=  WEBNAME;
      webpage += "' method='POST'>";
      webpage += "<button>Manual</button></form><br>";
      webpage += "</form>";
      webpage += "</body>";
      mServer.send(200, "text/html", webpage);
    }

    void FWUpdate() {      
      HTTPUpload& upload = mServer.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.setDebugOutput(true);
        WiFiUDP::stopAll();
        DebugOut::debug_out("Update:" + upload.filename);
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace)) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
          DebugOut::debug_out("Update success: " + upload.totalSize);
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      }
      yield();     
    }
  private:
    ESP8266WebServer mServer;
    Settings& mData;
};

#endif
