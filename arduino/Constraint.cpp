
#include "../automation/constraint/Constraint.h"

#include "JsonWriter.h"

namespace automation {

  void Constraint::print(int depth) const {
    JsonSerialWriter w(depth);
    w.noPrefixPrintln("{");
    w.increaseDepth();
    w.printlnStringObj(F("title"), getTitle().c_str() , ",");
    w.printlnStringObj(F("state"), isPassed() ? "PASSED" : "FAILED", ",");
    w.printlnStringObj(F("type"), getType().c_str() , ",");
    w.printStringObj(F("mode"), Constraint::modeToString(mode).c_str());
    w.decreaseDepth();
    w.noPrefixPrintln("");
    w.print("}");
  }

}

