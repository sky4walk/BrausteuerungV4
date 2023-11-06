// brausteuerung@AndreBetz.de
#ifndef __SENSOR__
#define __SENSOR__

#include <Arduino.h>
#include <math.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "settings.h"

///////////////////////////////////////////////////////////////////////////////
// Temperatur Sensoren DS18B20
///////////////////////////////////////////////////////////////////////////////
class TemperaturSensorDS18B20
{
    // DS18B20
    // DALLES 18B50 CONNECTION
    // Dallas     | waterproof | Arduino
    // ----------------------------------
    // PIN 1 GND  |  black     | GND
    // PIN 2 Data |  yellow    | D12
    // PIN 3 VCC  |  red       | 5V
    //   _______
    //  /  TOP  \
    // /_________\
    //    | | |
    //    1 2 3
    // 4.7KOhm zwischen PIN 2 und PIN 3
  public:
    TemperaturSensorDS18B20(
      uint8_t pin,
      Settings& set) :
      mPin(pin),
      mOneWire(mPin),
      mSensor(&mOneWire),
      mSettings(set)
    {
    }
    void begin() {
      mSensor.begin();
      mSensor.setResolution(11);
      mSensor.setWaitForConversion(false);
      mSensor.requestTemperatures();
    }
    float getTemperatur()
    {
      float val = mSensor.getTempCByIndex(0);
      mSensor.requestTemperatures();
      if ( -126 > val ) {
        setup();
      }      
      return val * mSettings.getKalM() + mSettings.getKalT();
    }
  private:
    uint8_t mPin;
    OneWire mOneWire;
    DallasTemperature mSensor;
    Settings& mSettings;
};

#endif
