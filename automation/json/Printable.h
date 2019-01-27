#ifndef _AUTOMATION_JSON_PRINTABLE_H_
#define _AUTOMATION_JSON_PRINTABLE_H_

#include "JsonStreamWriter.h"

namespace automation
{
namespace json
{
  //TODO - typedef some types for type checking args at compile time in call to print()
const bool PrefixOn = true;
const bool PrefixOff = false;
const bool VerboseOn = true;
const bool VerboseOff = false;

struct Printable
{

  void print(int depth = 0) const
  {
    JsonSerialWriter w(depth);
    print(w, false);
  }

  void printVerbose(int depth = 0) const
  {
    JsonSerialWriter w(depth);
    print(w, true);
  }

  void noPrefixPrint(JsonStreamWriter &w, bool bVerbose = false) const
  {
    print(w, true, false);
  }

  virtual void print(JsonStreamWriter &w, bool bVerbose = false, bool bIncludePrefix = true) const = 0;
};

} // namespace json
} // namespace automation
#endif