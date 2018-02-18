#ifndef __WEBMENU__
#define __WEBMENU__

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <FS.h>
#include "settings.h"

const char* BODY_STYLE = "<style>body { background-color: #E6E6FA; font-family: Arial, Helvetica, Sans-Serif; Color: blue;}</style>";

class WebMenu {
  public:
/*----------------------------------------------------------------------------------*/    
  
    WebMenu(Settings& data)
      :	mServer(80), mData(data) {
    }
/*----------------------------------------------------------------------------------*/    
    void init() {
      mServer.on("/",    std::bind(&WebMenu::startMenu, this));
      mServer.on("/sw",  std::bind(&WebMenu::setUpWifi, this));
     
      mServer.onNotFound([this]() {
        mServer.send(404, "text/plain", "404: Not found");
      });
    }
/*----------------------------------------------------------------------------------*/    
    void startServer() {
      if ( MDNS.begin(DNSNAME) ) {
        mHttpUpdater.setup(&mServer);
        mServer.begin();
        MDNS.addService("http", "tcp", 80);
        DebugOut::debug_out(DNSNAME);
      } else {
        mHttpUpdater.setup(&mServer);
        mServer.begin();
        DebugOut::debug_out("No DNS");
      }
    }
    void polling() {
      mServer.handleClient();
    }
/*----------------------------------------------------------------------------------*/    
  private:
/*----------------------------------------------------------------------------------*/    
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
      webpage += "<form action='/update' method='GET'>";
      webpage += "<button>FW update</button></form><br>";
      webpage += "<form action='http://";
      webpage +=  DNSNAME;
      webpage += "' method='POST'>";
      webpage += "<button>Manual</button></form><br>";
      webpage += "</form>";
      webpage += "</body>";
      mServer.send(200, "text/html", webpage);
    }
/*----------------------------------------------------------------------------------*/    
    void setUpWifi() {
      DebugOut::debug_out("setUpWifi");
      int n = WiFi.scanNetworks();

      String webpage = "";
      webpage =  "<html><head><title>Wifi Setup</title>";
      webpage += BODY_STYLE;
      webpage += "</head><body>";
      webpage += "<h1>Access points</h1>";

      webpage += "<form action=\"/sw\" method=\"POST\">";              
      for (int i = 0; i < n; ++i)
      {
        webpage += "<button type=\"submit\" name=\"ssid_in\" value=\"";
        webpage += WiFi.SSID(i) + "\">";
        webpage += WiFi.SSID(i);
        webpage += " (";
        webpage += WiFi.RSSI(i);
        webpage += ")";
        webpage += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
        webpage += "</button><br>";
      }
      webpage += "</form>";
      
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
/*----------------------------------------------------------------------------------*/        
  private:
    ESP8266WebServer mServer;
    ESP8266HTTPUpdateServer mHttpUpdater;
    Settings& mData;
};

#endif
