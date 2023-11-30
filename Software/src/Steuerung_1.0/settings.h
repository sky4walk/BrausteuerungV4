// brausteuerung@AndreBetz.de
#ifndef __SETTINGS__
#define __SETTINGS__

#include "DbgConsole.h"

#define MAXRAST             16
#define TEMPRESOLUTION      12
#define PIDOWINTERVAL       (1000 / (1 << (12 - TEMPRESOLUTION)))
#define PIDWINDOWSIZE       (PIDOWINTERVAL * 6)
#define PIDMINWINDOW        500

///////////////////////////////////////////////////////////////////////////////
// settings
///////////////////////////////////////////////////////////////////////////////
class Settings
{
  public:
    Settings()
    {
      params.mKalT              = 0.0f;
      params.mKalM              = 1.0f;
      params.pidKp              = 5000.0f;
      params.pidKi              = 1000.0f;
      params.pidKd              = 0.0f;      
      params.PidOWinterval      = PIDOWINTERVAL;
      params.PidWindowSize      = PIDWINDOWSIZE;
      params.PidMinWindow       = PIDMINWINDOW;
      params.switchProtocol     = 1;
      params.switchPulseLength  = 315;
      params.switchRepeat       = 15;
      params.switchBits         = 24;
      params.switchOn           = 1631343;
      params.switchOff          = 1631342;
      params.actRast            = 0;
      params.started            = false;
      params.heatState          = false;
      params.heatStateChanged   = false;
      params.UseDefault         = true;
      params.configMode         = true;      
      params.tempReached        = false;
      params.shouldSave         = false;
      params.restartEsp         = false;
      
      for ( int i = 0; i < MAXRAST; i++ )
      {
        params.rasten[i].temp = i;
        params.rasten[i].time = i;
      }
    }
    int getMAXRAST() {
      return MAXRAST;
    }
    float getKalT() 
    {
      return params.mKalT;
    }
    void setKalT(float kaltT) 
    {
      params.mKalT = kaltT;
    }
    float getKalM() 
    {
      return params.mKalM;
    }
    void setKalM(float kaltM) 
    {
      params.mKalM = kaltM;
    }
    float getPidKp() 
    {
      return params.pidKp;
    }
    void setPidKp(float pidKp) 
    {
      params.pidKp = pidKp;
    }
    float getPidKi() 
    {
      return params.pidKi;
    }
    void setPidKi(float pidKi) 
    {
      params.pidKi = pidKi;
    }
    float getPidKd() 
    {
      return params.pidKd;
    }
    void setPidKd(float pidKd) 
    {
      params.pidKd = pidKd;
    }
    unsigned long getPidOWinterval()
    {
      return params.PidOWinterval;
    }
    void setPidOWinterval(unsigned long PidOWinterval)
    {
      params.PidOWinterval = PidOWinterval;
    }
    unsigned long getPidWindowSize()
    {
      return params.PidWindowSize;
    }
    void setPidWindowSize(unsigned long PidWindowSize)
    {
      params.PidWindowSize = PidWindowSize;
    }
    unsigned long getPidMinWindow()
    {
      return params.PidMinWindow;
    }
    void setPidMinWindow(unsigned long PidMinWindow)
    {
      params.PidMinWindow = PidMinWindow;
    }
    double getPidOutput() {
      return params.pidOutput;
    }
    void setPidOutput(double pidOutput) {
      params.pidOutput = pidOutput;
    }
    void setWebPassWd(String webPassWd) {
      params.webPassWd = webPassWd;
    }
    String getWebPassWd() {
      return params.webPassWd;
    }
    float getActTemp(){
      return params.actTemp;
    }
    void setActTemp(float actTemp){
      params.actTemp = actTemp;
    }
    bool getShouldSave() {
      return params.shouldSave;
    }
    void setShouldSave(bool shouldSave) {
      params.shouldSave = shouldSave;
    }
    String getPassWd() {
      return params.passWd;
    }
    void setPassWd(String passWd) {
      params.passWd = passWd;
    }
    int getSwitchOn() {
      return params.switchOn;
    }
    void setSwitchOn(int switchOn) {
      params.switchOn = switchOn;
    }
    int getSwitchOff() {
      return params.switchOff;
    }
    void setSwitchOff(int switchOff) {
      params.switchOff = switchOff;
    }
    int getSwitchBits() {
      return params.switchBits;
    }
    void setSwitchBits(int switchBits) {
      params.switchBits = switchBits;
    }
    int getSwitchProtocol() {
      return params.switchProtocol;
    }
    void setSwitchProtocol(int switchProtocol) {
      params.switchProtocol = switchProtocol;
    }
    int getSwitchPulseLength() {
      return params.switchPulseLength;
    }
    void setSwitchPulseLength(int switchPulseLength) {
      params.switchPulseLength = switchPulseLength;
    }
    int getSwitchRepeat() {
      return params.switchRepeat;
    }
    void setSwitchRepeat(int switchRepeat) {
      params.switchRepeat = switchRepeat;
    }
    int getActRast() {
      return params.actRast;
    }
    void setActRast(int actRast) {
      params.actRast = actRast;
    }
    int getActState() {
      return params.actState;
    }
    void setActState(int actState) {
      params.actState = actState;
    }
    bool getStarted()
    {
      return params.started;
    }
    void setStarted(bool started)
    {
      params.started = started;
    }
    bool getHeatState()
    {
      return params.heatState;
    }
    void setHeatState(bool heatState)
    {
      params.heatState = heatState;
    }
    bool getHeatStateChanged()
    {
      return params.heatStateChanged;
    }
    void setHeatStateChanged(bool heatStateChanged)
    {
      params.heatStateChanged = heatStateChanged;
    }   
    bool getTempReached()
    {
      return params.tempReached;
    }
    void setTempReached(bool tempReached)
    {
      params.tempReached = tempReached;
    }
    bool getUseDefault()
    {
      return params.UseDefault;
    }
    void setUseDefault(bool UseDefault)
    {
      params.UseDefault = UseDefault;
    }
    bool getConfigMode() {
      return params.configMode;
    }
    void setConfigMode(bool configMode) {
      params.configMode = configMode;
    }
    bool getRestartEsp() {
      return params.restartEsp;
    }
    void setRestartEsp(bool restartEsp) {
      params.restartEsp = restartEsp;
    }
    unsigned long getTime(unsigned int nr) {
      if ( nr < MAXRAST )
        return params.rasten[nr].time;
      else
        return -1;
    }
    void setTime(unsigned int nr, unsigned long  time) {
      if ( nr < MAXRAST )
        params.rasten[nr].time = time;
    }
    int getTemp(unsigned int nr) {
      if ( nr < MAXRAST )
        return params.rasten[nr].temp;
      else
        return -1;
    }
    void setTemp(unsigned int nr, int  temp) {
      if ( nr < MAXRAST )
        params.rasten[nr].temp = temp;
    }
    unsigned long getDuration() {
      return params.duration;
    }
    void setDuration(unsigned long duration) {
      params.duration = duration;
    }
    bool getActive(unsigned int nr) {
      if ( nr < MAXRAST )
        return params.rasten[nr].active;
      else
        return false;
    }
    void setActive(unsigned int nr, bool  active) {
      if ( nr < MAXRAST )
        params.rasten[nr].active = active;
    }
    bool getWait(unsigned int nr) {
      if ( nr < MAXRAST )
        return params.rasten[nr].wait;
      else
        return false;
    }
    void setWait(unsigned int nr, bool  wait) {
      if ( nr < MAXRAST )
        params.rasten[nr].wait = wait;
    }
    bool getAlarm(unsigned int nr) {
      if ( nr < MAXRAST )
        return params.rasten[nr].alarm;
      else
        return false;
    }
    void setAlarm(unsigned int nr, bool  alarm) {
      if ( nr < MAXRAST )
        params.rasten[nr].alarm = alarm;
    }
    void clear(){
      memset((char*)&params, 0, sizeof(params));
    }
    void printRast(int nr) {
      if ( nr < MAXRAST ) {
        CONSOLELN(nr);
        CONSOLELN(getTime(nr));
        CONSOLELN(getTemp(nr));
        CONSOLELN(getActive(nr));
        CONSOLELN(getWait(nr));
        CONSOLELN(getAlarm(nr));
      }
    }
  private:
    
    typedef struct
    {
      unsigned long  time;
      int temp;
      bool active;
      bool wait;
      bool alarm;
    } Braurast;
    struct Rezept
    {
      Braurast rasten[MAXRAST];
      float mKalT;
      float mKalM;
      float pidKp;
      float pidKi;
      float pidKd;
      unsigned long PidOWinterval;
      unsigned long PidWindowSize;
      unsigned long PidMinWindow;
      double pidOutput;
      int switchProtocol;
      int switchPulseLength;
      int switchRepeat;
      int switchBits;
      unsigned long switchOn;
      unsigned long switchOff;
      int actRast;
      int actState;
      float actTemp;
      unsigned long duration;
      bool started;
      bool heatState;
      bool heatStateChanged;
      bool tempReached;
      bool UseDefault;
      bool shouldSave;
      bool configMode;
      bool restartEsp;
      String webPassWd;
      String passWd;      
    } params;
};
#endif
