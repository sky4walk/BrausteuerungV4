//C:\Users\andre\Documents\Arduino\libraries
#include <RCSwitch.h>
#include <ezBuzzer.h> 
#include "DbgConsole.h"
#include "tempsensor.h" 
#include "WaitTime.h"

#define GPIO16_D0 16
#define GPIO05_D1  5
#define GPIO04_D2  4
#define GPIO00_D3  0
#define GPIO02_D4  2
#define GPIO14_D5 14
#define GPIO12_D6 12
#define GPIO13_D7 13
#define GPIO15_D8 15
#define LOOPTIME 1000

Settings datas;
RCSwitch mySwitch = RCSwitch();
TemperaturSensorDS18B20 tmpSensor(GPIO04_D2,datas);
ezBuzzer buzzer(GPIO00_D3);
WaitTime innerLoop(LOOPTIME);
bool state = false;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  delay(200);

  CONSOLELN(F("Start"));
  CONSOLELN(ESP.getSdkVersion());
  pinMode(LED_BUILTIN, OUTPUT);
  
  mySwitch.enableTransmit(GPIO15_D8);
  mySwitch.setProtocol(datas.getSwitchProtocol());
  mySwitch.setPulseLength(datas.getSwitchPulseLength()); 
}

void loop() {
  buzzer.loop();
  innerLoop.start();
  if (innerLoop.timeOver()) {
    float temperatureC = tmpSensor.getTemperatur();
    if ( -126.0f < temperatureC ) {
       CONSOLE(temperatureC);
    }
    if( state ){
      digitalWrite(BUILTIN_LED, LOW);
      CONSOLELN(" on");
      mySwitch.send(datas.getSwitchOn(),datas.getSwitchBits());
      buzzer.beep(100);
      state = false; 
    } else {
      digitalWrite(BUILTIN_LED, HIGH);
      CONSOLELN(" off");
      mySwitch.send(datas.getSwitchOff(),datas.getSwitchBits());          
      state = true; 
    }
    innerLoop.restart();      
  }
}
