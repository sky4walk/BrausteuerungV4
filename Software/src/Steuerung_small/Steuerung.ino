//#include <DNSServer.h>
//#include <ESP8266WebServer.h>
//#include <ESP8266WiFi.h>
#include <RCSwitch.h>
#include "DbgConsole.h"
#include "tempsensor.h"
#include "settings.h"
#include "controllerHysterese.h"
#include "Led.h"
#include "DoubleResetDetector.h"
#include "WaitTime.h"
#include "SettingsLoader.h"
#include "WifiCofiguration.h"
#include "TempWebServer.h"

#define GPIO5_D1 5
#define GPIO4_D2 4
#define GPIO2_D4 2
#define DRD_ADDRESS 0
#define DRD_TIMEOUT 10
#define READ_TEMP  1*MIL2SEC

bool configMode = true;
Settings datas;
RCSwitch mySwitch = RCSwitch();
TemperaturSensorDS18B20 tmpSensor(GPIO4_D2,datas);
ControllerHysterese controlTmp(datas);
Led led(GPIO5_D1);
DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);
WaitTime sensorRead(READ_TEMP);
SettingsLoader loader(datas);
WifiConfiguration wifiConfig(datas);
TempWebServer rmpServer(wifiConfig.getWebserver(),&datas);

void setup() {
  Serial.begin(115200);
  while (!Serial);

  delay(200);
  
  CONSOLELN(ESP.getSdkVersion());
  pinMode(LED_BUILTIN, OUTPUT);

  wifiConfig.begin();
  
  configMode = false;

  if ( !loader.load() ) {
    loader.save();
  }

  if ( drd.detectDoubleReset() ) {
    CONSOLELN("Start web config");
    digitalWrite(LED_BUILTIN, LOW);
    configMode = true;
    wifiConfig.config();
    loader.save();
  } else {
    CONSOLELN("No Double Reset Detected");   
    if (!wifiConfig.connect()) {
    digitalWrite(LED_BUILTIN, LOW);
      configMode = true;
      loader.save();
    }
    digitalWrite(LED_BUILTIN, HIGH);
  }

  rmpServer.begin();
  tmpSensor.begin();
  controlTmp.begin();
  led.begin();
  mySwitch.enableTransmit(0); //D3
  mySwitch.setProtocol(datas.getSwitchProtocol());
  mySwitch.setPulseLength(datas.getSwitchPulseLength());  
//  mySwitch.setRepeatTransmit(datas.getSwitchRepeat()); 
}

void loop() {
  if ( ! configMode ) {
    sensorRead.start();
    if (sensorRead.timeOver()) {
      float temperatureC = tmpSensor.getTemperatur();
      if ( -126.0f < temperatureC ) {
        bool st = controlTmp.getState(temperatureC);
        led.set(st);
        datas.setActTemp(temperatureC);
        datas.setOnOff(st);
        CONSOLE(temperatureC);
        CONSOLE("ºC");
        if (st ) {
          CONSOLELN(" on");
          mySwitch.send(datas.getSwitchOn(),datas.getSwitchBits());
        } else {
          CONSOLELN(" off");
          mySwitch.send(datas.getSwitchOff(),datas.getSwitchBits());
        }
        sensorRead.restart();
      }
    }
    if ( datas.getShouldSave() ) {
      loader.save();
      datas.setShouldSave(false);
    }
  }
  drd.loop();
  if ( datas.getRestartEsp() ) {
    delay(500);
    ESP.restart();
  }
}
