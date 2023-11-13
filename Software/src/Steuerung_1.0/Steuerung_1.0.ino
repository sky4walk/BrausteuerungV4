// brausteuerung@AndreBetz.de
///////////////////////////////////////////////
//http://brausteuerung
///////////////////////////////////////////////
#if defined(ESP8266)
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#else
#include <WiFi.h>
#endif
#include <ESP8266WebServer.h>
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
#include "TempWebServer.h"
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
#define STATE_BEGIN   0
#define STATE_START   1
#define STATE_BREW    10
///////////////////////////////////////////////////////////////////////////////
// variablen
///////////////////////////////////////////////////////////////////////////////
double pidOutput                 = 0;
double actTmp                    = 25;
double sollTmp                   = 25;
Settings datas;
SettingsLoader loader(datas);
RCSwitch mySwitch = RCSwitch();
TemperaturSensorDS18B20 tmpSensor(GPIO04_D2,datas);
ezBuzzer buzzer(GPIO00_D3);
DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);
PID myPID(&actTmp,&pidOutput,&sollTmp,datas.getPidKp(),datas.getPidKi(),datas.getPidKd(),DIRECT);
ESP8266WebServer server(80);
TempWebServer rmpServer(server,datas);
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
  myPID.SetMode(MANUAL);
  pidOutput = 0;
  myPID.SetMode(AUTOMATIC);
  CONSOLELN(F("PIDres"));
}
///////////////////////////////////////////////////////////////////////////////
// setPID
///////////////////////////////////////////////////////////////////////////////
void setPID() {
  myPID.SetOutputLimits(datas.getPidMinWindow(), datas.getPidWindowSize());
  myPID.SetTunings(datas.getPidKp(), datas.getPidKi(), datas.getPidKd());
  myPID.SetSampleTime(datas.getPidOWinterval());
  resetPID();
}
///////////////////////////////////////////////////////////////////////////////
// Relais
///////////////////////////////////////////////////////////////////////////////
void Relais(bool onOff)
{
  if ( onOff ) {
    CONSOLELN(F("On"));
    mySwitch.send(datas.getSwitchOn(), datas.getSwitchBits());
  } else {
    CONSOLELN(F("Off"));
    mySwitch.send(datas.getSwitchOff(), datas.getSwitchBits());
  }
}
///////////////////////////////////////////////////////////////////////////////
// compute PID in loop
///////////////////////////////////////////////////////////////////////////////
void PidLoop() {
  timerPidCompute.start();
  if ( timerPidCompute.timeOver() ) {
    timerPidCompute.restart();
    actTmp = datas.getActTemp();
    myPID.Compute();
    datas.setPidOutput(pidOutput);
    CONSOLE(F("PIDcomp:"));
    CONSOLELN(pidOutput);
    pidRelaisTimer.setTime(datas.getPidOutput());
    pidRelaisTimer.start();
  }
}
///////////////////////////////////////////////////////////////////////////////
// detect state of heater is changed
///////////////////////////////////////////////////////////////////////////////
void changeHeatState(bool onOff) {
  if ( onOff != datas.getHeatState() )
    datas.setHeatStateChanged(true);
  datas.setHeatState(onOff);
}
///////////////////////////////////////////////////////////////////////////////
// detect state of heater is changed
///////////////////////////////////////////////////////////////////////////////
void HeatLoop() {
  if ( ( false == pidRelaisTimer.timeOver() ) &&
       ( datas.getPidOutput() > datas.getPidMinWindow()  ) ) {
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
    datas.setActTemp(tmpSensor.getTemperatur());
    CONSOLELN(datas.getActTemp()); 
  }
}
///////////////////////////////////////////////////////////////////////////////
// detect state of heater is changed
///////////////////////////////////////////////////////////////////////////////
void RelaisLoop(){
  timerSendHeatState.start();
  if ( timerSendHeatState.timeOver() || datas.getHeatStateChanged() ) {
    timerSendHeatState.restart();
    datas.setHeatStateChanged(false);
    Relais( datas.getHeatState() );
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

  if(!loader.load())
    loader.save();

  WiFiManager wifiManager;
  if ( drd.detectDoubleReset() ) {
    CONSOLELN(F("web cfg"));
    wifiManager.resetSettings();
  } else {
    CONSOLELN(F("No DRD"));   
  }
  wifiManager.setAPStaticIPConfig(IPAddress(10,0,0,1), IPAddress(10,0,0,1), IPAddress(255,255,255,0));
  wifiManager.autoConnect("Brausteuerung"); 

  while (WiFi.status() != WL_CONNECTED) 
  {
    CONSOLELN(F("."));
    delay(500);
  }
  WiFi.hostname("Brausteuerung");
  CONSOLELN(WiFi.localIP());
  if (MDNS.begin("Brausteuerung"))   {  
    CONSOLELN(F("DNS started"));  

  }
  
  ArduinoOTA.begin();
  
  mySwitch.enableTransmit(GPIO15_D8);
  mySwitch.setProtocol(datas.getSwitchProtocol());
  mySwitch.setPulseLength(datas.getSwitchPulseLength()); 

  timerPidCompute.setTime(datas.getPidOWinterval());
  timerTempMeasure.setTime(datas.getPidWindowSize());
  timerSendHeatState.setTime(datas.getPidWindowSize());

  datas.setActState(STATE_BEGIN); 

  setPID();

  LedTicker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, LOW);
  rmpServer.begin();
  server.begin();
  CONSOLELN(F("Srv run"));
}
///////////////////////////////////////////////////////////////////////////////
// main loop
///////////////////////////////////////////////////////////////////////////////
void loop() {
  ArduinoOTA.handle();
  TempLoop();
  switch(datas.getActState()) {
    case STATE_BEGIN:
    break;
    case STATE_BREW:
    {
      HeatLoop();
    }
    break;
    default:
    break;
  }
  RelaisLoop();
  drd.loop();
  server.handleClient(); // https://www.youtube.com/watch?v=n1_uCypHofU
  if ( datas.getRestartEsp() ) {
    delay(500);
    ESP.restart();
  }
}
