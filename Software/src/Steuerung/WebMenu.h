#ifndef __WEBMENU__
#define __WEBMENU__

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include "settings.h"

const char* BODY_STYLE = "<style>body { background-color: #E6E6FA; font-family: Arial, Helvetica, Sans-Serif; Color: blue;}</style>";

class WebMenu {
  public:
    WebMenu(Settings& data)
      :	mServer(80), mData(data) {
    }
    void init() {
      mServer.on("/",    std::bind(&WebMenu::startMenu, this));
      mServer.on("/sw",  std::bind(&WebMenu::setUpWifi, this));
      /*
            mServer.on("/ss",     HTTP_GET,  std::bind(&WebMenu::setUpWifiGet, this));
            mServer.on("/re",     HTTP_GET,  std::bind(&WebMenu::setUpWifiGet, this));
            mServer.on("/br",     HTTP_GET,  std::bind(&WebMenu::setUpWifiGet, this));
            mServer.on("/fu", [this]() {
              DebugOut::debug_out("setUpWifi");
              String webpage = "<form method='POST' action='/fud' enctype='multipart/form-data'>";
              webpage += "Firmware Version: ";
              webpage += String(FW_VERSION) + "<br>";
              webpage += "<input type='file' name='update'>";
              webpage += "<input type='submit' value='Update'></form>";
              mServer.send(200, "text/html", webpage );
            });

            mServer.on("/fud", HTTP_POST, [this]() {
              DebugOut::debug_out("download");
              mServer.sendHeader("Connection", "close");
              mServer.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
              ESP.restart();
            }, std::bind(&WebMenu::FWUpdate, this) );
      */
      mServer.onNotFound([this]() {
        mServer.send(404, "text/plain", "404: Not found");
      });

    }
    void startServer() {
      if ( MDNS.begin(DNSNAME) ) {
        MDNS.addService("http", "tcp", 80);
        DebugOut::debug_out(DNSNAME);
      } else {
        DebugOut::debug_out("No DNS");
      }
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
      webpage =  "<html><head><title>Wifi Setup</title>";
      webpage += BODY_STYLE;
      webpage += "</head><body>";
      webpage += "<h1>Access points</h1>";
      for (int i = 0; i < n; ++i)
      {
        webpage += "<form action=\"/sw\" method=\"GET\"> <button>";
        webpage += WiFi.SSID(i);
        webpage += " (";
        webpage += WiFi.RSSI(i);
        webpage += ")";
        webpage += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
        webpage += "</button></form>";
      }
      webpage += "<form action=\"/sw\" method=\"POST\">";
      webpage += "SSID:<input type='text' name=\"ssid_in\" ";
      webpage += "value=\"" +mData.getSSID()+"\"><br>";
      webpage += "PW:<input type='text' name=\"pw_in\" ";
      webpage += "value=\"" +mData.getPASS()+"\"><br>";
      webpage += "<input type=\"submit\" value=\"Enter\"><br>"; 
      webpage += "</form>";
      
      webpage += "<form action=\"/\" method=\"POST\">";
      webpage += "<input type=\"submit\" value=\"Back\">";
      webpage += "</form></body></html>";
      
      mServer.send(200, "text/html", webpage);
      if (mServer.args() > 0 ) {
        for ( uint8_t i = 0; i < mServer.args(); i++ ) {
          String srvArgName = mServer.argName(i);
          String srvArg = mServer.arg(i);
          DebugOut::debug_out(srvArgName);
          DebugOut::debug_out(srvArg);
          if (srvArgName == "ssid_in") {          
             mData.setSSID(srvArg);
          }
          if (srvArgName == "pw_in") {
             mData.setPASS(srvArg);             
          }
        }
      }
    }
    void startMenu() {
      DebugOut::debug_out("startMenu");
      String webpage = "";
      webpage =  "<html><head><title>mikroSikaru.de Brausteuerung V4</title>";
      webpage += BODY_STYLE;
      webpage += "</head><body><h1>Main Menu</h1>";
      webpage += "<form action='/sw' method='GET'>";
      webpage += "<button>Setup Wifi</button></form><br>";
      webpage += "<form action='/ss' method='GET'>";
      webpage += "<button>Setup Switch</button></form><br>";
      webpage += "<form action='/re' method='GET'>";
      webpage += "<button>Recipe</button></form><br>";
      webpage += "<form action='/br' method='GET'>";
      webpage += "<button>Brew</button></form><br>";
      webpage += "<form action='/fu' method='GET'>";
      webpage += "<button>FW update</button></form><br>";
      webpage += "<form action='http://";
      webpage +=  DNSNAME;
      webpage += "' method='POST'>";
      webpage += "<button>Manual</button></form><br>";
      webpage += "</form>";
      webpage += "</body>";
      mServer.send(200, "text/html", webpage);
    }

    void FWUpdate() {
      HTTPUpload& upload = mServer.upload();
      if (upload.status == UPLOAD_FILE_START) {
        DebugOut::debug_out("Update:" + upload.filename);
        Serial.setDebugOutput(true);
        WiFiUDP::stopAll();
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace)) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        DebugOut::debug_out("UW");
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        DebugOut::debug_out("UE");
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
