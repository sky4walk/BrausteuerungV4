// brausteuerung@AndreBetz.de
#ifndef __LED__
#define __LED__

///////////////////////////////////////////////////////////////////////////////
// LED
///////////////////////////////////////////////////////////////////////////////

class Led
{
  public:
    Led(uint8_t pin) 
    {
        mPin = pin;        
    }
    void begin()
    {
      pinMode(mPin, OUTPUT);
    }    
    void set(bool on)
    {
      if ( on ) {
        state = true;
        digitalWrite(mPin, HIGH);
      } else {
        state = false;
        digitalWrite(mPin, LOW);
      }
    }
    void toggle() {
      if ( state ) {
        set( !state );
      } else {
        set( state );        
      }
    }
    private:
      uint8_t mPin;
      bool state;
};

#endif
