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

  virtual void print(JsonStreamWriter &w, bool bVerbose = false, bool bIncludePrefix = true) const = 0;

  template<typename TKey>
  Printable& printObj(JsonStreamWriter &w, TKey k, const char* suffix = "", bool bVerbose = false )
  {     
    w.printKey(k);
    print(w,bVerbose,false);
    w.noPrefixPrint(suffix);
    return *this;
  }

  template<typename TKey>
  Printable& printlnObj(JsonStreamWriter &w, TKey k, const char* suffix = "", bool bVerbose = false )
  {     
    printObj(w,k,suffix,bVerbose);
    w.println();
    return *this;
  }

};

} // namespace json
} // namespace automation
#endif
