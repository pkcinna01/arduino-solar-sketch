#ifndef AUTOMATION_H
#define AUTOMATION_H

#include <string>
#include <sstream>

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

}
#endif
