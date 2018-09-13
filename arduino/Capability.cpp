
#include "../automation/capability/Capability.h"

#include "JsonWriter.h"

namespace automation {

void Capability::print(int depth)
{
    JsonSerialWriter w(depth);
    w.println("{");
    w.increaseDepth();
    w.printlnStringObj(F("type"),getType().c_str(),",");
    w.printlnStringObj(F("title"),getTitle().c_str(),",");
    w.printlnNumberObj(F("value"), getValue() );
    w.decreaseDepth();
    w.print("}");
  }

}

