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

  class NullStream : public std::ostream {
    class NullBuffer : public std::streambuf {
    public:
      int overflow( int c ) { return c; }
    } m_nb;
    public:
    NullStream() : std::ostream( &m_nb ) {}
  };

  std::ostream& getLogBufferImpl() {

    //std::cout.setstate(std::ios_base::badbit);
    //return std::cout;

    static NullStream nullStream;
    return nullStream;
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
}

