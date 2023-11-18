// brausteuerung@AndreBetz.de
#ifndef __STEUERUNGWEBSERVER__
#define __STEUERUNGWEBSERVER__

#include "settings.h"



class SteuerungWebServer {
  public:
    SteuerungWebServer(Settings& set);
    void begin();
  private:
    //handleFileUpload(AsyncWebServerRequest *request);
    Settings& mSettings;        
};

#endif
