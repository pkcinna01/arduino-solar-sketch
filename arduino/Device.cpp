
#include "../automation/device/Device.h"

#include "JsonWriter.h"

namespace automation {

  void Device::print(int depth, bool bVerbose) {
    JsonSerialWriter w(depth);
    w.println("{");
    w.increaseDepth();
    w.printlnStringObj(F("name"),name.c_str(),",");
    w.printlnStringObj(F("type"),getType().c_str(),",");    
    w.printKey(F("constraint"));
    if ( bVerbose ) {
        pConstraint->printVerbose(depth+1);
        w.noPrefixPrintln(",");
        w.printVectorObj(F("capabilities"), capabilities);
        printVerboseExtra(depth+1);
    } else {
        w.noPrefixPrint("{ \"") + F("state\": ") + (pConstraint->isPassed() ? "PASSED" : "FAILED");
        w.noPrefixPrintln(" }");
    }    
    w.decreaseDepth();
    w.print("}");
  }

  void Device::printVerboseExtra(int depth) {
    // override this to insert device specifics into printVerbose output
    JsonSerialWriter w(depth);
    w.noPrefixPrintln("");
  }
}
