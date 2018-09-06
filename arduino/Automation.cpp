//
// All arduino specific implementations for automation namespace here for now...
//

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
    return std::cout;
  }

  void clearLogBuffer() {
    cout.clear();
  }
  void logBufferToString( string& strDest ) {
    // not applicable to arduino and cout (serial port)
  }

}

