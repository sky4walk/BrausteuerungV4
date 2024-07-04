// brausteuerung@AndreBetz.de
#ifndef __SENSOR__
#define __SENSOR__

#include <Arduino.h>
#include <math.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "settings.h"
#define MAXVALUES         20
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
      mGradientPos(0),
      mSettings(set)
    {
    }
    void begin() {
      mSensor.begin();
      mSensor.setResolution(mSettings.getTempRes());
      mSensor.setWaitForConversion(false);
      mSensor.requestTemperatures();
    }
    float getTemperatur()
    {
      sensorConnected = true;
      float val = mSensor.getTempCByIndex(0);
      mSensor.requestTemperatures();
      if ( DEVICE_DISCONNECTED_C == val)
      {
        sensorConnected = false;
        CONSOLELN(F("sensor disc"));
      }
      return val * mSettings.getKalM() + mSettings.getKalT();
    }
    bool getSensorConnected() {
      return sensorConnected;
    }
    void resetStored(float val) {
      mGradientPos = 0;
      for ( int i = 0; i < MAXVALUES; i++ )
        mStored[i] = val;
    }
    void addVal(float val) {
      mStored[mGradientPos] = val;
      mGradientPos++;
      if ( MAXVALUES <= mGradientPos )
        mGradientPos = 0;
    }
    float getActVal() {
      if ( 0 == mGradientPos) {
        return mStored[MAXVALUES - 1];
      } else {
        return mStored[mGradientPos - 1];
      }
    }
    float getGradient() {
      // Temperaturabfrage alle TIMER_TEMP_MEASURE sekunde ist
      // und der Gradient auf 1C/Min berechnet werden soll
      
      float res = (getActVal() - mStored[mGradientPos]) * 
                  ( 60000 / MAXVALUES / mTimerTempMeasure);
      
      return res;
    }
    void setTimerTempMeasure(long timerTempMeasure) {
      if ( timerTempMeasure > 0)
        mTimerTempMeasure = timerTempMeasure;
      else
        mTimerTempMeasure = MAXVALUES;
    }
    byte getGradientPos() {
      return mGradientPos;
    }
  private:
    uint8_t mPin;
    bool sensorConnected;
    OneWire mOneWire;
    DallasTemperature mSensor;
    float mStored[MAXVALUES];
    byte mGradientPos;
    long mTimerTempMeasure;
    Settings& mSettings;
};

#endif
