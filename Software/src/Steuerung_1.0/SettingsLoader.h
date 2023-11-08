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
      CONSOLELN(size);
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
      if ( doc.containsKey("pidKp") )
        mSettings.setPidKp( doc["pidKp"] );
      if ( doc.containsKey("pidKi") )
        mSettings.setPidKi( doc["pidKi"] );
      if ( doc.containsKey("pidKd") )
        mSettings.setPidKd( doc["pidKd"] );
      if ( doc.containsKey("PidOWinterval") )
        mSettings.setPidOWinterval( doc["PidOWinterval"] );
      if ( doc.containsKey("PidWindowSize") )
        mSettings.setPidWindowSize( doc["PidWindowSize"] );
      if ( doc.containsKey("PidMinWindow") )
        mSettings.setPidMinWindow( doc["PidMinWindow"] );
      if ( doc.containsKey("switchProtocol") )
        mSettings.setSwitchProtocol( doc["switchProtocol"] );
      if ( doc.containsKey("switchPulseLength") )
        mSettings.setSwitchPulseLength( doc["switchPulseLength"] );
      if ( doc.containsKey("switchRepeat") )
        mSettings.setSwitchRepeat( doc["switchRepeat"] );
      if ( doc.containsKey("switchBits") )
        mSettings.setSwitchBits( doc["switchBits"] );
      if ( doc.containsKey("switchOn") )
        mSettings.setSwitchOn( doc["switchOn"] );
      if ( doc.containsKey("switchOff") )
        mSettings.setSwitchOff( doc["switchOff"] );
      if ( doc.containsKey("actRast") )
        mSettings.setActRast( doc["actRast"] );
      if ( doc.containsKey("actTemp") )
        mSettings.setActTemp( doc["actTemp"] );
      if ( doc.containsKey("started") )
        mSettings.setStarted( doc["started"] );
      if ( doc.containsKey("heatState") )
        mSettings.setHeatState( doc["heatState"] );
      if ( doc.containsKey("tempReached") )
        mSettings.setTempReached( doc["tempReached"] );
      if ( doc.containsKey("UseDefault") )
        mSettings.setUseDefault( doc["UseDefault"] );
      if ( doc.containsKey("shouldSave") )
        mSettings.setShouldSave( doc["shouldSave"] );
      if ( doc.containsKey("restartEsp") )
        mSettings.setRestartEsp( doc["restartEsp"] );
      if ( doc.containsKey("setPassWd") )
        mSettings.setWebPassWd( doc["setPassWd"] );
      if ( doc.containsKey("passWd") )
        mSettings.setPassWd( doc["passWd"] );

      for ( int i=0; i < mSettings.getMAXRAST(); i++ ) {
         JsonObject posRast = doc[F("Rast_")+String(i)];
         if ( !posRast.isNull() ){
           if ( posRast.containsKey("time") ) 
              mSettings.setTime(i,posRast["time"]);
           if ( posRast.containsKey("temp") ) 
              mSettings.setTemp(i,posRast["temp"]);
           if ( posRast.containsKey("active") ) 
              mSettings.setActive(i,posRast["active"]);
           if ( posRast.containsKey("wait") ) 
              mSettings.setWait(i,posRast["wait"]);
           if ( posRast.containsKey("alarm") ) 
              mSettings.setAlarm(i,posRast["alarm"]);
         }
        
         mSettings.printRast(i);
      }  

      serializeJsonPretty(doc, Serial);

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
      
      doc["kalT"]               = mSettings.getKalT();
      doc["kalM"]               = mSettings.getKalM();
      doc["pidKp"]              = mSettings.getPidKp();
      doc["pidKi"]              = mSettings.getPidKi();
      doc["pidKd"]              = mSettings.getPidKd();
      doc["PidOWinterval"]      = mSettings.getPidOWinterval();
      doc["PidWindowSize"]      = mSettings.getPidWindowSize();
      doc["PidMinWindow"]       = mSettings.getPidMinWindow();
      doc["switchProtocol"]     = mSettings.getSwitchProtocol();
      doc["switchPulseLength"]  = mSettings.getSwitchPulseLength();
      doc["switchRepeat"]       = mSettings.getSwitchRepeat();
      doc["switchBits"]         = mSettings.getSwitchBits();
      doc["switchOn"]           = mSettings.getSwitchOn();
      doc["switchOff"]          = mSettings.getSwitchOff();
      doc["actRast"]            = mSettings.getActRast();
      doc["actTemp"]            = mSettings.getActTemp();
      doc["started"]            = mSettings.getStarted();
      doc["heatState"]          = mSettings.getHeatState();
      doc["tempReached"]        = mSettings.getTempReached();
      doc["UseDefault"]         = mSettings.getUseDefault();
      doc["shouldSave"]         = mSettings.getShouldSave();
      doc["restartEsp"]         = mSettings.getRestartEsp();
      doc["setPassWd"]          = mSettings.getWebPassWd();
      doc["passWd"]             = mSettings.getPassWd();

      for ( int i=0; i < mSettings.getMAXRAST(); i++ ) {
        JsonObject posRast  = doc.createNestedObject(F("Rast_")+String(i));
        posRast["time"] = mSettings.getTime(i);
        posRast["temp"] = mSettings.getTemp(i);
        posRast["active"] = mSettings.getActive(i);
        posRast["wait"] = mSettings.getWait(i);
        posRast["alarm"] = mSettings.getAlarm(i);
      }

      serializeJsonPretty(doc, Serial);
      
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