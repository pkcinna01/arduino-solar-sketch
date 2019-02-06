#ifndef _AUTOMATION_JSON_H_
#define _AUTOMATION_JSON_H_

#include "../Automation.h"

namespace automation
{
namespace json
{

enum class JsonFormat { INVALID = -1, COMPACT, PRETTY };

static JsonFormat jsonFormat = JsonFormat::PRETTY;

static JsonFormat parseFormat(const char *pszFormat)
{
  if (!strcasecmp_P(pszFormat, PSTR("COMPACT")))
  {
    return JsonFormat::COMPACT;
  }
  else if (!strcasecmp_P(pszFormat, PSTR("PRETTY")))
  {
    return JsonFormat::PRETTY;
  }
  else
  {
    return JsonFormat::INVALID;
  }
}

static string formatAsString(JsonFormat fmt)
{
  if (fmt == JsonFormat::COMPACT)
  {
    return RVSTR("COMPACT");
  }
  else if (fmt == JsonFormat::PRETTY)
  {
    return RVSTR("PRETTY");
  }
  else
  {
    string msg(RVSTR("INVALID:"));
    msg += (unsigned int)fmt;
    return msg;
  }
}

static bool isPretty() { return jsonFormat == JsonFormat::PRETTY; }

} // namespace json
} // namespace automation
#endif