// brausteuerung@AndreBetz.de
#ifndef __CONTROLLERHYSTERESE__
#define __CONTROLLERHYSTERESE__

#include <Arduino.h>
#include "settings.h"
///////////////////////////////////////////////////////////////////////////////
// ControlLerHysterese
///////////////////////////////////////////////////////////////////////////////

class ControllerHysterese
{
  public:
    ControllerHysterese(Settings& set) :
      mSettings(set)
      {
        
      }
    void begin() 
    {
      mState = false;      
    }
    bool getState(float actTmp) {
      if ( isHeater() ){
        CONSOLE(" Heater ");
      } else {
        CONSOLE(" Cooler ");       
      }
      CONSOLE(actTmp);
      CONSOLE(" on ");
      CONSOLE(mSettings.getOnT());
      CONSOLE(" off ");
      CONSOLELN(mSettings.getOffT());
      
      if ( actTmp >= mSettings.getOffT() ) {
         mState = false;
      } else if ( actTmp <= mSettings.getOnT() ) {
         mState = true;
      }
      return isHeater() ?  mState : !mState;
    }
  private:
    bool isHeater()
    {
      if (mSettings.getOffT() > mSettings.getOnT() )
      {
        return true;
      } else {
        return false;
      }            
    }
    Settings& mSettings;
    bool mState;
};

#endif
