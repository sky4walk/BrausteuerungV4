// brausteuerung@AndreBetz.de
#ifndef __SETTINGS__
#define __SETTINGS__

#include "DbgConsole.h"

#define MAXRAST             16
#define TEMPRESOLUTION      11
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
      reset();
    }
    void reset() {
      params.mKalT              = 0.0f;
      params.mKalM              = 1.0f;
      params.pidKp              = 5000.0f;
      params.pidKi              = 1000.0f;
      params.pidKd              = 0.0f;
      params.tempRes            = TEMPRESOLUTION;      
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
      params.startRast          = 0;
      params.actShowRast        = 0;
      params.shouldStart        = false;
      params.playSound          = false;
      params.rastWait           = false;
      params.started            = false;
      params.heatState          = false;
      params.heatStateChanged   = false;
      params.cooling            = false;
      params.UseDefault         = true;
      params.configMode         = true;      
      params.tempReached        = false;
      params.shouldSave         = false;
      params.restartEsp         = false;
      params.useAP              = true;
      params.resetWM            = false;
      params.DNSEntry           = "Brausteuerung";
      params.info               = "";
      params.maxRast            = MAXRAST;
      params.actTime            = "";
      
      for ( int i = 0; i < MAXRAST; i++ )
      {
        params.rasten[i].temp  = i;
        params.rasten[i].time  = i;
        params.rasten[i].alarm = true;
        params.rasten[i].name  = "Rast"+String(i+1);
        params.rasten[i].info  = "";
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
    int getTempRes() {
      return params.tempRes;
    }
    unsigned long getPidOWinterval() {
      return params.PidOWinterval;
    }
    void setPidOWinterval(unsigned long PidOWinterval) {
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
    bool getResetWM() {
      return params.resetWM;
    }
    void setResetWM(bool resetWM) {
      params.resetWM = resetWM;
    }
    bool getUseAP() {
      return params.useAP;
    }
    void setUseAP(bool useAP) {
      params.useAP = useAP;
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
    int getStartRast() {
      return params.startRast;
    }
    void setStartRast(int startRast) {
      params.startRast = startRast;
    }
    int getActShowRast() {
      return params.actShowRast;
    }
    void setActShowRast(int actShowRast) {
      params.actShowRast = actShowRast;
    }
    int getActState() {
      return params.actState;
    }
    void setActState(int actState) {
      params.actState = actState;
    }
    int getActBrewState() {
      return params.actBrewState;
    }
    void setActBrewState(int actBrewState) {
      params.actBrewState = actBrewState;
    }
    bool getPlaySound()
    {
      return params.playSound;
    }
    void setPlaySound(bool playSound)
    {
      params.playSound = playSound;
    }
    bool getRastWait()
    {
      return params.rastWait;
    }
    void setRastWait(bool rastWait)
    {
      params.rastWait = rastWait;
    }
    bool getShouldStart()
    {
      return params.shouldStart;
    }
    void setShouldStart(bool shouldStart)
    {
      params.shouldStart = shouldStart;
    }
    bool getStarted()
    {
      return params.started;
    }
    void setStarted(bool started)
    {
      params.started = started;
    }
    bool getShouldResetState()
    {
      return params.shouldResetState;
    }
    void setShouldResetState(bool shouldResetState)
    {
      params.shouldResetState = shouldResetState;
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
     bool getCooling()
    {
      return params.cooling;
    }
    void setCooling(bool cooling)
    {
      params.cooling = cooling;
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
    String getDNSEntry() {
      return params.DNSEntry;
    }
    void setDNSEntry(String dnsEntry) {
      params.DNSEntry = dnsEntry;
    }
    String getInfo() {
        return params.info;
    }
    void setInfo(String info) {
        params.info = info;
    }
    String getActTime() {
      return params.actTime;
    }
    void setActTime(String actTime) {
      params.actTime = actTime;
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
    String getName(unsigned int nr) {
      if ( nr < MAXRAST )
        return params.rasten[nr].name;
      else
        return "";
    }
    void setName(unsigned int nr, String name) {
      if ( nr < MAXRAST )
        params.rasten[nr].name = name;
    }
    String getInfo(unsigned int nr) {
      if ( nr < MAXRAST )
        return params.rasten[nr].info;
      else
        return "";
    }
    void setInfo(unsigned int nr, String info) {
      if ( nr < MAXRAST )
        params.rasten[nr].info = info;
    }
    void clear(){
      memset((char*)&params, 0, sizeof(params));
    }
    void printRast(int nr) {
      if ( nr < MAXRAST ) {
        CONSOLELN(nr);
        CONSOLELN(getName(nr));
        CONSOLELN(getTime(nr));
        CONSOLELN(getTemp(nr));
        CONSOLELN(getActive(nr));
        CONSOLELN(getWait(nr));
        CONSOLELN(getAlarm(nr));
        CONSOLELN(getInfo(nr));
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
      String name;
      String info;
    } Braurast;
    struct Rezept
    {
      Braurast rasten[MAXRAST];
      float mKalT;
      float mKalM;
      float pidKp;
      float pidKi;
      float pidKd;
      int tempRes;
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
      int actShowRast;
      int startRast;
      int actState;
      int actBrewState;
      float actTemp;
      unsigned long duration;
      int maxRast;
      bool shouldStart;
      bool playSound;
      bool rastWait;
      bool started;
      bool heatState;
      bool heatStateChanged;
      bool cooling;
      bool tempReached;
      bool UseDefault;
      bool shouldSave;
      bool shouldResetState;
      bool configMode;
      bool restartEsp;
      bool resetWM;
      bool useAP;
      String info;
      String DNSEntry;
      String webPassWd;
      String passWd;
      String actTime;      
    } params;
};
#endif
