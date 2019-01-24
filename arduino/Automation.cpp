//
// All arduino specific implementations for automation namespace here for now...
//

#include "Arduino.h"
#include "../automation/Automation.h"

#include <iostream>

using namespace std;

namespace automation {

  unsigned long millisecs() {
    return millis();
  }

  void sleep(unsigned long intervalMs) {
    delay(intervalMs);
  }

  std::ostream& getLogBufferImpl() {

    //std::cout.setstate(std::ios_base::badbit);
    //return std::cout;

    
    return automation::cnull;
  }

  void clearLogBuffer() {
    //cout.clear();
  }

  void logBufferToString( string& strDest ) {
    // not applicable to arduino and cout (serial port)
  }

  bool isTimeValid() {
    return timeStatus() == timeSet;
  }

  void threadKeepAliveReset() { watchdog::keepAlive(); }
}

