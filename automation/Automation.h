#ifndef AUTOMATION_H
#define AUTOMATION_H

#include "text.h"

#include <vector>
#include <iostream>

using namespace std;

#ifdef ARDUINO_APP
  #define RTTI_GET_TYPE_DECL virtual string getType() const = 0;
  #define RTTI_GET_TYPE_IMPL(package,className) \
  string getType() const override { return String(F(#className)).c_str(); };
#else
  #define RTTI_GET_TYPE_DECL \
    virtual string getType() const = 0;\
    virtual string getPackage() const = 0;\
    virtual string getFullType() const = 0;
  #define RTTI_GET_TYPE_IMPL(package,className) \
    string getType() const override { return #className; };\
    string getPackage() const override { return #package; };\
    string getFullType() const override { return #package "::" #className; };
#endif

namespace automation {

  const unsigned int MINUTES = 60000;
  const unsigned int SECONDS = 1000;

  class NullStream : public std::ostream {
    class NullBuffer : public std::streambuf {
    public:
      int overflow( int c ) { return c; }
    } m_nb;
    public:
    NullStream() : std::ostream( &m_nb ) {}
  };

  static NullStream cnull;
  
  std::ostream& getLogBufferImpl();
  void clearLogBuffer();
  void logBufferToString( string& strDest );

  static std::ostream& logBuffer = getLogBufferImpl();

  static bool bSynchronizing = false;

  unsigned long millisecs();

  // TODO: review calls to 32 bit millisecs() and consider 64 bit version if rollover impacts computations
  #ifdef ARDUINO_APP
  static uint64_t millisecs64() { 
    static uint64_t msbs = 0;
    static uint64_t lastMillis = 0;
    unsigned long ms = millisecs();
    if ( ms < lastMillis ) {
      // assume rollover...
      msbs++;
    }
    lastMillis = ms;
    uint64_t rtn = msbs << 32;
    rtn += ms;
    return rtn;
  }
  #else
  // TODO: need precompile check for size of unsigned long (assume 64 bits for now)
  static uint64_t millisecs64() {
    return millisecs();
  }
  #endif

  //template <typename TimerVal>
  typedef uint64_t TimerVal;

  class Timer {
    public:
    Timer(TimerVal startTimeMs = 0) : 
      startTimeMs(startTimeMs)
    {      
    }
    
    void expire() { startTimeMs = 0; }

    void reset() { startTimeMs = millisecs64(); }

    bool haveStartTime() const { return startTimeMs != 0; }

    bool isExpiredMs(TimerVal durationMs) const {
      return !haveStartTime() || getElapsedDurationMs() > durationMs;
    }

    TimerVal getElapsedDurationMs() const { 
      return getElapsedDurationMs(millisecs64());
    }

    TimerVal getElapsedDurationMs(TimerVal sinceTimeMs) const { 
      return sinceTimeMs - startTimeMs;
    }

    protected:
    TimerVal startTimeMs;
  };

  //template <typename TimerVal>
  class DurationTimer : public Timer {
    public:
    DurationTimer(TimerVal maxDurationMs, TimerVal startTimeMs = 0) : 
      Timer(startTimeMs),
      maxDurationMs(maxDurationMs) 
    {      
    }
    
    bool isExpired() const { return Timer::isExpiredMs(maxDurationMs); }
    TimerVal getMaxDurationMs() const { return maxDurationMs; }
    DurationTimer& setMaxDurationMs(TimerVal maxDurationMs) { this->maxDurationMs = maxDurationMs; return *this; }
    DurationTimer& setMaxDurationAsSeconds(TimerVal maxDurationSeconds) { this->maxDurationMs = maxDurationMs*1000; return *this; }

    protected:
    TimerVal maxDurationMs;

  };

  void sleep(unsigned long intervalMs);

  bool isTimeValid(); // handle arduino with time never set

  void threadKeepAliveReset();

  // If an external client is managing state we need to know if it is still connected.  When
  // connection is lost, devices must revert to internal constraints to know when it is time
  // to power off or change state
  namespace client { 
    namespace watchdog {
    const unsigned long KeepAliveExpireDurationMs = 2L*MINUTES;
    static unsigned long keepAliveTimeMs = millisecs();
    static unsigned long messageReceived() { 
      keepAliveTimeMs = millisecs(); 
      return keepAliveTimeMs; 
    }
    static bool isKeepAliveExpired() { 
      unsigned long elapsedTimeMs = millisecs()-keepAliveTimeMs;
      return elapsedTimeMs > KeepAliveExpireDurationMs; 
    }
  }};

}
#endif
