

#include "Capability.h"
#include "../json/JsonStreamWriter.h"

namespace automation {

void Capability::print(json::JsonStreamWriter& w, bool bVerbose, bool bIncludePrefix) const
{
    if ( bIncludePrefix ) w.println("{"); else w.noPrefixPrintln("{");

    w.increaseDepth();
    w.printlnStringObj(F("type"),getType().c_str(),",");
    w.printlnStringObj(F("title"),getTitle().c_str(),",");
    w.printlnNumberObj(F("value"), getValue() );
    w.decreaseDepth();
    w.print("}");
  }

}

