#ifndef _AUTOMATION_JSON_H_
#define _AUTOMATION_JSON_H_

#include "../Automation.h"

namespace automation
{
namespace json
{

typedef unsigned char JsonFormat;

const JsonFormat JSON_FORMAT_INVALID = -1, JSON_FORMAT_COMPACT = 0, JSON_FORMAT_PRETTY = 1;

static JsonFormat jsonFormat = JSON_FORMAT_PRETTY;

static JsonFormat parseFormat(const char *pszFormat)
{
  if (!strcasecmp_P(pszFormat, PSTR("COMPACT")))
  {
    return JSON_FORMAT_COMPACT;
  }
  else if (!strcasecmp_P(pszFormat, PSTR("PRETTY")))
  {
    return JSON_FORMAT_PRETTY;
  }
  else
  {
    return JSON_FORMAT_INVALID;
  }
}

static string formatAsString(JsonFormat fmt)
{
  if (fmt == JSON_FORMAT_COMPACT)
  {
    return "COMPACT";
  }
  else if (fmt == JSON_FORMAT_PRETTY)
  {
    return "PRETTY";
  }
  else
  {
    string msg("INVALID:");
    msg += (unsigned int)fmt;
    return msg;
  }
}

static bool isPretty() { return jsonFormat == JSON_FORMAT_PRETTY; }

} // namespace json
} // namespace automation
#endif