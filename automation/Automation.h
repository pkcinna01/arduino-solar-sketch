#ifndef AUTOMATION_H
#define AUTOMATION_H

#include <string.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

using namespace std;

#ifdef ARDUINO_APP
  #define RVSTR(str) String(F(str)).c_str()
  #define RTTI_GET_TYPE_DECL virtual string getType() const = 0;
  #define RTTI_GET_TYPE_IMPL(package,className) \
  string getType() const override { return String(F(#className)).c_str(); };
#else
  // create no-op string functions where arduino has flash memory strings
  #define F(str) str
  #define PSTR(str) str
  #define RVSTR(str) str
  #define strcasecmp_P strcasecmp
  #define strncasecmp_P strncasecmp
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

  std::ostream& getLogBufferImpl();
  void clearLogBuffer();
  void logBufferToString( string& strDest );

  static std::ostream& logBuffer = getLogBufferImpl();

  static bool bSynchronizing = false;

  template<typename T>
  string asString(const T &t) {
    ostringstream os;
    os << t;
    return os.str();
  }

  static bool parseBool(const char* pszVal) {
    return !strcasecmp_P(pszVal, PSTR("ON")) 
      || !strcasecmp_P(pszVal, PSTR("TRUE")) 
      || !strcasecmp_P(pszVal, PSTR("YES"))
      || atoi(pszVal) > 0;
  }

  unsigned long millisecs();

  void sleep(unsigned long intervalMs);

  bool isTimeValid(); // handle arduino with time never set

  class AttributeContainer {
    public:
    mutable std::string name;
    AttributeContainer(const std::string& name) : name(name) {}
    virtual bool setAttribute(const char* pszKey, const char* pszVal, ostream* pResponseStream = nullptr) {
      if ( pszKey && !strcasecmp("name",pszKey) ) {
        name = pszVal;
      } else {
        return false;
      }
      if ( pResponseStream ) {
        (*pResponseStream) << "'" << name << "' " << pszKey << "=" << pszVal;
      }
      return true; // attribute found and set
    }
    virtual const string& getTitle() const {
      return name;
    }
  };

  struct WildcardMatcher {

    string strPattern;

    WildcardMatcher(const char* strPattern) :
        strPattern(strPattern) {
    }

    bool test(const char *pszSubject) {
      return WildcardMatcher::test(strPattern.c_str(),pszSubject);
    }

    static bool test(const char* pszPattern, const char* pszSubject) {
      const char* star=NULL;
      const char* ss=pszSubject;
      while (*pszSubject){
        if ((*pszPattern=='?')||(tolower(*pszPattern)==tolower(*pszSubject))){pszSubject++;pszPattern++;continue;}
        if (*pszPattern=='*'){star=pszPattern++; ss=pszSubject;continue;}
        if (star){ pszPattern = star+1; pszSubject=++ss;continue;}
        return false;
      }
      while (*pszPattern=='*'){pszPattern++;}
      return !*pszPattern;
    }
  };

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
          if (bInclude==WildcardMatcher::test(namePattern.c_str(),item->name.c_str()) ) {
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
