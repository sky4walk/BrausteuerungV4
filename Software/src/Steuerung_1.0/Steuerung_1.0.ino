// brausteuerung@AndreBetz.de
///////////////////////////////////////////////
//http://brausteuerung
//C:\Users\skype\OneDrive\Dokumente\Arduino\libraries
//https://github.com/sui77/rc-switch 2.6.4
//https://arduinogetstarted.com/tutorials/arduino-buzzer-library 1.0.0
//https://github.com/tzapu/WiFiManager 2.0.16-rc.2
//PID 1.2.0
//https://www.pjrc.com/teensy/td_libs_OneWire.html 2.3.7
//https://github.com/dvarrel/ESPAsyncTCP 1.2.4
//https://github.com/datacute/DoubleResetDetector 1.0.3
//https://github.com/milesburton/Arduino-Temperature-Control-Library 3.9.0
//https://github.com/dvarrel/AsyncTCP 1.1.4
//https://arduinojson.org/ 6.21.3
//https://github.com/me-no-dev/ESPAsyncWebServer 
// ask for HW type: esptool.exe -p COM4 flash_id
///////////////////////////////////////////////
#if defined(ESP8266)
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#else
#include <WiFi.h>
#endif
#include <ESP8266mDNS.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <RCSwitch.h>
#include <ezBuzzer.h> 
#include <ArduinoOTA.h>
#include <PID_v1.h>
#include <Ticker.h>
#include "DoubleResetDetector.h"
#include "DbgConsole.h"
#include "tempsensor.h" 
#include "WaitTime.h"
#include "settings.h"
#include "SettingsLoader.h"
#include "SteuerungWebServer.h"
///////////////////////////////////////////////
// HW defines
///////////////////////////////////////////////
#define GPIO16_D0 16
#define GPIO05_D1  5
#define GPIO04_D2  4
#define GPIO00_D3  0
#define GPIO02_D4  2
#define GPIO14_D5 14
#define GPIO12_D6 12
#define GPIO13_D7 13
#define GPIO15_D8 15
#define DRD_ADDRESS 0
#define DRD_TIMEOUT 10
///////////////////////////////////////////////////////////////////////////////
// states
///////////////////////////////////////////////////////////////////////////////
#define STATE_BEGIN       0
#define STATE_START       1
#define STATE_BREW        10
#define STATE_TMPREACHED  20
#define STATE_FIN         30
#define MIL2MIN           60 * 1000
///////////////////////////////////////////////////////////////////////////////
// variablen
///////////////////////////////////////////////////////////////////////////////
double pidOutput                 = 0;
double actTmp                    = 25;
double sollTmp                   = 25;
Settings brewDatas;
SettingsLoader loaderDat(brewDatas);
RCSwitch mySwitch = RCSwitch();
TemperaturSensorDS18B20 tmpSensor(GPIO04_D2,brewDatas);
ezBuzzer buzzer(GPIO00_D3);
DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);
PID myPID(&actTmp,&pidOutput,&sollTmp,brewDatas.getPidKp(),brewDatas.getPidKi(),brewDatas.getPidKd(),DIRECT);
SteuerungWebServer rmpServer(&brewDatas);
Ticker LedTicker;
WaitTime          timerTempMeasure;
WaitTime          timerPidCompute;
WaitTime          timerSendHeatState;
WaitTime          timerBrewTimer;
WaitTime          pidRelaisTimer;
///////////////////////////////////////////////////////////////////////////////
// tikcer led 
///////////////////////////////////////////////////////////////////////////////
void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}
///////////////////////////////////////////////////////////////////////////////
// resetPID
///////////////////////////////////////////////////////////////////////////////
void resetPID() {
  timerPidCompute.setTime(brewDatas.getPidWindowSize());
  myPID.SetMode(MANUAL);
  pidOutput = 0;
  myPID.SetMode(AUTOMATIC);
  CONSOLELN(F("PIDres"));
}
///////////////////////////////////////////////////////////////////////////////
// setPID
///////////////////////////////////////////////////////////////////////////////
void setPID() {
  myPID.SetOutputLimits(brewDatas.getPidMinWindow(), brewDatas.getPidWindowSize());
  myPID.SetTunings(brewDatas.getPidKp(), brewDatas.getPidKi(), brewDatas.getPidKd());
  myPID.SetSampleTime(brewDatas.getPidOWinterval());
  resetPID();
}
///////////////////////////////////////////////////////////////////////////////
// Relais
///////////////////////////////////////////////////////////////////////////////
void Relais(bool onOff)
{
  if ( onOff ) {
    CONSOLELN(F("On"));
    mySwitch.send(brewDatas.getSwitchOn(), brewDatas.getSwitchBits());
  } else {
    CONSOLELN(F("Off"));
    mySwitch.send(brewDatas.getSwitchOff(), brewDatas.getSwitchBits());
  }
}
///////////////////////////////////////////////////////////////////////////////
// compute PID in loop
///////////////////////////////////////////////////////////////////////////////
void PidLoop() {
  timerPidCompute.start();
  if ( timerPidCompute.timeOver() ) {
    timerPidCompute.restart();
    actTmp = brewDatas.getActTemp();
    myPID.Compute();
    brewDatas.setPidOutput(pidOutput);
    CONSOLE(F("PIDcomp:"));
    CONSOLELN(pidOutput);
    pidRelaisTimer.setTime(brewDatas.getPidOutput());
    pidRelaisTimer.start();
  }
}
///////////////////////////////////////////////////////////////////////////////
// detect state of heater is changed
///////////////////////////////////////////////////////////////////////////////
void changeHeatState(bool onOff) {
  if ( onOff != brewDatas.getHeatState() )
    brewDatas.setHeatStateChanged(true);
  brewDatas.setHeatState(onOff);
}
///////////////////////////////////////////////////////////////////////////////
// detect state of heater is changed
///////////////////////////////////////////////////////////////////////////////
void HeatLoop() {
  if ( ( false == pidRelaisTimer.timeOver() ) &&
       ( brewDatas.getPidOutput() > brewDatas.getPidMinWindow()  ) ) {
    changeHeatState(true);
  } else {
    changeHeatState(false);
  }
}
///////////////////////////////////////////////////////////////////////////////
// read Temp
///////////////////////////////////////////////////////////////////////////////
void TempLoop() {
  timerTempMeasure.start();
  if ( timerTempMeasure.timeOver() ) {
    timerTempMeasure.restart();       
    brewDatas.setActTemp(tmpSensor.getTemperatur());
    CONSOLELN(brewDatas.getActTemp());     
  }
}
///////////////////////////////////////////////////////////////////////////////
// detect state of heater is changed
///////////////////////////////////////////////////////////////////////////////
void RelaisLoop(){
  timerSendHeatState.start();
  if ( timerSendHeatState.timeOver() || brewDatas.getHeatStateChanged() ) {
    timerSendHeatState.restart();
    brewDatas.setHeatStateChanged(false);
    Relais( brewDatas.getHeatState() );
  }
}
///////////////////////////////////////////////////////////////////////////////
// setup
///////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  while (!Serial);

  delay(200);

  CONSOLELN(F("start"));
  CONSOLELN(ESP.getSdkVersion());
  pinMode(LED_BUILTIN, OUTPUT);

  LedTicker.attach(0.6, tick);

  if(!loaderDat.load())
    loaderDat.save();
  
  if ( drd.detectDoubleReset() ) {
    CONSOLELN(F("ddr"));
    brewDatas.setResetWM(true);
    if ( brewDatas.getUseAP() ) {
      brewDatas.setUseAP(false);
    } else {
      brewDatas.setUseAP(true);
    }
    loaderDat.save();
  } else {
    CONSOLELN(F("No ddr"));   
  }

  if ( brewDatas.getUseAP() ) {
    CONSOLELN(F("AP Mode"));
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(IPAddress(10,0,0,1), IPAddress(10,0,0,1), IPAddress(255,255,255,0));
    WiFi.softAP("Brausteuerung", "");
    CONSOLELN(WiFi.softAPIP());
  } else {
    CONSOLELN(F("STA Mode"));
    WiFiManager wm;
    if ( brewDatas.getResetWM() ) {
      CONSOLELN(F("reset WM"));
      brewDatas.setResetWM(false);
      wm.resetSettings();
    }
    wm.setAPStaticIPConfig(IPAddress(10,0,0,1), IPAddress(10,0,0,1), IPAddress(255,255,255,0));
    wm.setEnableConfigPortal(false);    
    if ( !wm.autoConnect("Brausteuerung") ) {
      delay(1000);
      CONSOLELN(F("not con"));
      wm.startConfigPortal("Brausteuerung");
      ESP.restart();
    } else {
      while (WiFi.status() != WL_CONNECTED) {
        CONSOLE(F("."));
        yield();
      }
      CONSOLELN(F("connected to WIFI"));
    }
  }

  WiFi.hostname("Brausteuerung");
  CONSOLELN(WiFi.localIP());
  if (MDNS.begin("Brausteuerung"))   {  
    CONSOLELN(F("DNS started"));  

  }

  //ArduinoOTA.setHostname("Brausteuerung");
  //ArduinoOTA.begin();
  
  mySwitch.enableTransmit(GPIO15_D8);
  mySwitch.setProtocol(brewDatas.getSwitchProtocol());
  mySwitch.setPulseLength(brewDatas.getSwitchPulseLength()); 

  timerPidCompute.setTime(brewDatas.getPidWindowSize());
  timerTempMeasure.setTime(brewDatas.getPidOWinterval());
  timerSendHeatState.setTime(brewDatas.getPidWindowSize());

  brewDatas.setActState(STATE_BEGIN); 

  setPID();

  LedTicker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, LOW);
  rmpServer.begin();
  CONSOLELN(F("Srv run"));
}
///////////////////////////////////////////////////////////////////////////////
// main loop
///////////////////////////////////////////////////////////////////////////////
void loop() {
  //ArduinoOTA.handle();
  buzzer.loop();
  TempLoop();
  int actRast = brewDatas.getActRast();
  switch(brewDatas.getActState()) {
    case STATE_BEGIN: {
      brewDatas.setPlaySound(brewDatas.getAlarm(actRast));
    }
    break;
    case STATE_BREW: {
      PidLoop();
      HeatLoop();
      if ( brewDatas.getActTemp( ) >= brewDatas.getTemp(actRast) ) {
        brewDatas.setActState(STATE_TMPREACHED);
        brewDatas.setTempReached(true);
        timerBrewTimer.setTime(brewDatas.getTime(actRast)*MIL2MIN);
        sollTmp = brewDatas.getTemp(actRast);
        resetPID();
        timerBrewTimer.start();
        CONSOLELN(F("STATE_TMPREACHED"));
        CONSOLELN(brewDatas.getTime(actRast));
      }
    }
    break;
    case STATE_TMPREACHED: {
      PidLoop();
      HeatLoop();
      brewDatas.setDuration(timerBrewTimer.getDuration());
      if ( timerBrewTimer.timeOver() ) {
        timerBrewTimer.resume();
        brewDatas.setActState(STATE_FIN);
        CONSOLELN(F("STATE_FIN"));
      }
    }
    break;
    case STATE_FIN: {
      PidLoop();
      HeatLoop();
      if (brewDatas.getPlaySound()) {
        buzzer.beep(100);
      }
    }
    break;
    default:
    break;
  }
  RelaisLoop();
  drd.loop();
  if ( brewDatas.getShouldSave() ) {
    loaderDat.save();
    brewDatas.setShouldSave(false);
  }
  if ( brewDatas.getShouldResetState() ) {
    CONSOLELN("getShouldResetState");
    changeHeatState(false);
    brewDatas.setTempReached(false);
    brewDatas.setStarted(false);
    brewDatas.setActState(STATE_BEGIN); 
    resetPID();
    timerBrewTimer.resume();
    brewDatas.setShouldResetState(false);
  }
  if ( brewDatas.getShouldStart() ) {
    CONSOLELN("getShouldStart");
    resetPID();
    brewDatas.setStarted(true);
    brewDatas.setActState(STATE_BREW);
    brewDatas.setShouldStart(false); 
  }
  if ( brewDatas.getRestartEsp() ) {
    delay(500);
    ESP.restart();
  }
}
