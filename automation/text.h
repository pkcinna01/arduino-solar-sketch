#ifndef _AUTOMATION_TEXT_H_
#define _AUTOMATION_TEXT_H_

#include <string.h>
#include <string>
#include <sstream>

#ifdef ARDUINO_APP
#define RVSTR(str) String(F(str)).c_str()
#else
// create no-op string functions where arduino has flash memory strings
#define F(str) str
#define PSTR(str) str
#define RVSTR(str) str
#define strcasecmp_P strcasecmp
#define strncasecmp_P strncasecmp
#endif

namespace automation
{
namespace text
{

template <typename T>
std::string asString(const T &t)
{
  std::ostringstream os;
  os << t;
  return os.str();
}

static bool parseBool(const char *pszVal)
{
  return !strcasecmp_P(pszVal, PSTR("ON")) || !strcasecmp_P(pszVal, PSTR("TRUE")) || !strcasecmp_P(pszVal, PSTR("YES")) || atoi(pszVal) > 0;
}

struct WildcardMatcher
{
  std::string strPattern;

  WildcardMatcher(const char *strPattern) : strPattern(strPattern)
  {
  }

  bool test(const char *pszSubject)
  {
    return WildcardMatcher::test(strPattern.c_str(), pszSubject);
  }

  static bool test(const char *pszPattern, const char *pszSubject)
  {
    const char *star = NULL;
    const char *ss = pszSubject;
    while (*pszSubject)
    {
      if ((*pszPattern == '?') || (tolower(*pszPattern) == tolower(*pszSubject)))
      {
        pszSubject++;
        pszPattern++;
        continue;
      }
      if (*pszPattern == '*')
      {
        star = pszPattern++;
        ss = pszSubject;
        continue;
      }
      if (star)
      {
        pszPattern = star + 1;
        pszSubject = ++ss;
        continue;
      }
      return false;
    }
    while (*pszPattern == '*')
    {
      pszPattern++;
    }
    return !*pszPattern;
  }
};

} // namespace text
} // namespace automation

#endif