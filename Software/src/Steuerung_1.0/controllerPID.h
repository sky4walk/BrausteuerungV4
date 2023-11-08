// brausteuerung@AndreBetz.de
#ifndef __CONTROLLERPID__
#define __CONTROLLERPID__

#include <Arduino.h>
#include <PID_v1.h>
#include "settings.h"
#include "WaitTime.h"

///////////////////////////////////////////////////////////////////////////////
// ControlLerPID
///////////////////////////////////////////////////////////////////////////////

class ControllerPID
{
  public:
    ControllerPID(Settings& set) :
      mSettings(set),
      myPid(
            &actTmp, 
            &pidOutput, 
            &sollTmp, 
            mSettings.getPidKp(), 
            mSettings.getPidKi(), 
            mSettings.getPidKd(), 
            DIRECT)
      {
        timerPidCompute.setTime(mSettings.getPidWindowSize());
      }
    void begin() 
    {
      mState = false;      
    }
    void setActTmp(double actTemp) {
      actTmp = actTmp;
    }
    void setSollTemp(double sollTemp) {
      sollTmp = sollTemp;
    }
    bool getState(float actTmp) {
      if ( isHeater() ){
        CONSOLE(" Heater ");
      } else {
        CONSOLE(" Cooler ");       
      }
      
      return isHeater() ?  mState : !mState;
    }
    void resetPID() {
      myPid.SetMode(MANUAL);
      pidOutput = 0;
      myPid.SetMode(AUTOMATIC);
      CONSOLELN(F("PIDres"));
    }
    void setPID() {
      myPid.SetOutputLimits(mSettings.getPidMinWindow(), mSettings.getPidWindowSize());
      myPid.SetTunings(mSettings.getPidKp(), mSettings.getPidKi(), mSettings.getPidKd());
      myPid.SetSampleTime(mSettings.getPidOWinterval());
      resetPID();
    }
    
    void loop() {
      setActTmp(mSettings.getActTemp());
      myPid.Compute();
      CONSOLE(F("PIDcomp:"));
      CONSOLELN(pidOutput);
    }
    
  private:
    PID myPid;
    double actTmp;
    double pidOutput;
    double sollTmp;
    WaitTime timerPidCompute;
    bool isHeater()
    {
      return true;
    }
    Settings& mSettings;
    bool mState;
};

#endif
