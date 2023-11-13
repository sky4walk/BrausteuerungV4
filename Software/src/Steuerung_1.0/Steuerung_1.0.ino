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
Settings brewDatas;
SettingsLoader loaderDat(brewDatas);
RCSwitch mySwitch = RCSwitch();
TemperaturSensorDS18B20 tmpSensor(GPIO04_D2,brewDatas);
ezBuzzer buzzer(GPIO00_D3);
DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);
PID myPID(&actTmp,&pidOutput,&sollTmp,brewDatas.getPidKp(),brewDatas.getPidKi(),brewDatas.getPidKd(),DIRECT);
ESP8266WebServer server(80);
//WebSocketsServer webSocket(81);
TempWebServer rmpServer(server, brewDatas);
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
  mySwitch.setProtocol(brewDatas.getSwitchProtocol());
  mySwitch.setPulseLength(brewDatas.getSwitchPulseLength()); 

  timerPidCompute.setTime(brewDatas.getPidOWinterval());
  timerTempMeasure.setTime(brewDatas.getPidWindowSize());
  timerSendHeatState.setTime(brewDatas.getPidWindowSize());

  brewDatas.setActState(STATE_BEGIN); 

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
  switch(brewDatas.getActState()) {
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
  rmpServer.loop();
  if ( brewDatas.getRestartEsp() ) {
    delay(500);
    ESP.restart();
  }
}
