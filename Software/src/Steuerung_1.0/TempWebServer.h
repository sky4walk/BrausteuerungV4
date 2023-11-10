// brausteuerung@AndreBetz.de
#ifndef __TEMPWEBSERVER__
#define __TEMPWEBSERVER__

#include <ESPAsyncWebServer.h>
#include "settings.h"


class TempWebServer {
  public:
    static Settings* mSettings;
    TempWebServer(AsyncWebServer& server,Settings* set);
    void begin();
  private:
    AsyncWebServer& mServer;
        
};

#endif
