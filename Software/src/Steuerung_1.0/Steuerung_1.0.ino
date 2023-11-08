// brausteuerung@AndreBetz.de
#include <RCSwitch.h>
#include <ezBuzzer.h> 
#include "DoubleResetDetector.h"
#include "DbgConsole.h"
#include "tempsensor.h" 
#include "WaitTime.h"
#include "settings.h"
#include "SettingsLoader.h"
#include "controllerPid.h"

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
#define LOOPTIME 1000

Settings datas;
SettingsLoader loader(datas);
RCSwitch mySwitch = RCSwitch();
TemperaturSensorDS18B20 tmpSensor(GPIO04_D2,datas);
ezBuzzer buzzer(GPIO00_D3);
WaitTime timerPidTemp;
DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);
ControllerPID controller(datas);

bool state = false;

void PidTempLoop(){
  timerPidTemp.start();
  if ( timerPidTemp.timeOver() )
  {
    timerPidTemp.restart();
    datas.setActTemp(tmpSensor.getTemperatur());
    controller.loop();
  }
}
void setup() {
  Serial.begin(115200);
  while (!Serial);

  delay(200);

  CONSOLELN(ESP.getSdkVersion());
  pinMode(LED_BUILTIN, OUTPUT);
  
  mySwitch.enableTransmit(GPIO15_D8);
  mySwitch.setProtocol(datas.getSwitchProtocol());
  mySwitch.setPulseLength(datas.getSwitchPulseLength()); 

  timerPidTemp.setTime(datas.getPidOWinterval());
  
  if(!loader.load())
    loader.save();

}

void loop() {
  PidTempLoop();
  drd.loop();
  if ( datas.getRestartEsp() ) {
    delay(500);
    ESP.restart();
  }
}
