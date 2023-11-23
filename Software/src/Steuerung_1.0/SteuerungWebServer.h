// brausteuerung@AndreBetz.de
#ifndef __STEUERUNGWEBSERVER__
#define __STEUERUNGWEBSERVER__

#include "settings.h"



class SteuerungWebServer {
  public:
    static Settings* mSettings;
    SteuerungWebServer(Settings* set);
    void begin();
  private:
};

#endif
