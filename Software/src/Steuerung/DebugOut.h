#ifndef __DEBUGOUT__
#define __DEBUGOUT__

namespace DebugOut {
  template<typename TArg>
  void debug_out(TArg msg){
#ifdef DEBUG_OUT
    Serial.println(msg);
#endif
  }
}

#endif

