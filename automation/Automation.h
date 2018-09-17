#ifndef AUTOMATION_H
#define AUTOMATION_H

#include <string>
#include <vector>
#include <sstream>
#include <iostream>

using namespace std;

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

  unsigned long millisecs();

  void sleep(unsigned long intervalMs);

  bool isTimeValid(); // handle arduino with time never set

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
        if ((*pszPattern=='?')||(*pszPattern==*pszSubject)){pszSubject++;pszPattern++;continue;}
        if (*pszPattern=='*'){star=pszPattern++; ss=pszSubject;continue;}
        if (star){ pszPattern = star+1; pszSubject=++ss;continue;}
        return false;
      }
      while (*pszPattern=='*'){pszPattern++;}
      return !*pszPattern;
    }
  };

  bool isMatch(const char *p, const char *s) {
    const char* star=NULL;
    const char* ss=s;
    while (*s){
      if ((*p=='?')||(*p==*s)){s++;p++;continue;}
      if (*p=='*'){star=p++; ss=s;continue;}
      if (star){ p = star+1; s=++ss;continue;}
      return false;
    }
    while (*p=='*'){p++;}
    return !*p;
  }

  template<typename T>
  class AutomationVector : public std::vector<T> {
  public:

    AutomationVector() : std::vector<T>(){}
    AutomationVector( vector<T>& v ) : std::vector<T>(v) {}
    std::vector<T>& filterByNames( const char* pszCommaDelimitedNames, std::vector<T>& resultVec ) {
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
          if (isMatch(namePattern.c_str(),item->name.c_str()) ) {
            resultVec.push_back(item);
            break;
          }
        }
      }
      return resultVec;
    };
  };
}
#endif
