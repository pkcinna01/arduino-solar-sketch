#ifndef AUTOMATION_H
#define AUTOMATION_H

#include "text.h"

#include <vector>
#include <sstream>
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

  void sleep(unsigned long intervalMs);

  bool isTimeValid(); // handle arduino with time never set

  void threadKeepAliveReset();

  template<typename T>
  class AutomationVector : public std::vector<T> {
  public:

    AutomationVector() : std::vector<T>(){}
    AutomationVector( vector<T>& v ) : std::vector<T>(v) {}
    std::vector<T>& filterByNames( const char* pszCommaDelimitedNames, std::vector<T>& resultVec, bool bInclude = true) {
      if ( pszCommaDelimitedNames == nullptr || strlen(pszCommaDelimitedNames) == 0 ) {
        pszCommaDelimitedNames = "*";
      }
      istringstream nameStream(pszCommaDelimitedNames);
      string str;
      vector<string> nameVec;
      while (std::getline(nameStream, str, ',')) {
        nameVec.push_back(str);
        if( nameStream.eof( ) ) {
          break;
        }
      }
      for( const T& item : *this ) {
        for( const string& namePattern : nameVec) {
          if (bInclude==text::WildcardMatcher::test(namePattern.c_str(),item->name.c_str()) ) {
            resultVec.push_back(item);
            break;
          }
        }
      }
      return resultVec;
    };
  };
  
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
