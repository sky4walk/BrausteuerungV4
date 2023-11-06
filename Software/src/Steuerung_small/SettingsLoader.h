// brausteuerung@AndreBetz.de
#ifndef __SETTINGSLOADER__
#define __SETTINGSLOADER__

#include <FS.h> 
#include <ArduinoJson.h>
#include "settings.h"
#include "DbgConsole.h"
#define SETTINGSFILE "/settings.json"

class SettingsLoader {
  public:
    SettingsLoader(Settings& set) :
       mSettings(set) {
      
    }
    bool load() {
      if (!SPIFFS.begin()) {
        CONSOLELN(F(" ERROR: failed to mount FS!"));
        return false;
      }       
      CONSOLELN(F(" mounted!"));

      if (!SPIFFS.exists(SETTINGSFILE)) {
        CONSOLELN(F("ERROR: failed to load json config"));
        return false;
      } 

      File configFile = SPIFFS.open(SETTINGSFILE, "r");
      if (!configFile)
      {
        CONSOLELN(F("ERROR: unable to open config file"));
        return false;
      }

      size_t size = configFile.size();
      DynamicJsonDocument doc(size * 3);
      DeserializationError error = deserializeJson(doc, configFile);
      if (error) {
          CONSOLE(F("deserializeJson() failed: "));
          CONSOLELN(error.c_str());
          return false;
      }

      if ( doc.containsKey("kalT") )
        mSettings.setKalT( doc["kalT"] );
      if ( doc.containsKey("kalM") )
        mSettings.setKalM( doc["kalM"] );
      if ( doc.containsKey("onT") )
        mSettings.setOnT( doc["onT"] );
      if ( doc.containsKey("offT") )
        mSettings.setOffT( doc["offT"] );

      return true;
    }
    bool format(){
      CONSOLE(F("\nneed to format SPIFFS: "));
      SPIFFS.end();
      SPIFFS.begin();
      CONSOLELN(SPIFFS.format());
      return SPIFFS.begin();
    }
    bool save() {
      if (!SPIFFS.begin())  {
        CONSOLELN("Failed to mount file system");
        if (!format()) {
          CONSOLELN("Failed to format file system - hardware issues!");
          return false;
        }        
      }      

      DynamicJsonDocument doc(2048);

      doc["kalT"] = mSettings.getKalT();
      doc["kalM"] = mSettings.getKalM();
      doc["onT"]  = mSettings.getOnT();
      doc["offT"] = mSettings.getOffT();
//      doc["SSID"] = WiFi.SSID();
//      doc["PSK"]  = WiFi.psk();

      File configFile = SPIFFS.open(SETTINGSFILE, "w");
      if (!configFile) {
        CONSOLELN(F("failed to open config file for writing"));
        SPIFFS.end();
        return false;
      }
  
      serializeJson(doc, configFile);
      configFile.flush();
      configFile.close();
      SPIFFS.gc();
      SPIFFS.end();
      CONSOLELN(F("\nsaved successfully"));
      return true;
  }

  private:
    Settings& mSettings;
};

#endif
