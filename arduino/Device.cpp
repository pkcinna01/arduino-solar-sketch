
#include "../automation/device/Device.h"

#include "JsonWriter.h"

namespace automation {

void Device::print(int depth)
{
    JsonSerialWriter w(depth);
    w.println("{");
    w.increaseDepth();
    w.printlnStringObj(F("name"),name.c_str(),",");
    w.printlnStringObj(F("constraint"), (bConstraintPassed || !pConstraint) ? "PASSED" : "FAILED" , ",");
    w.printlnVectorObj(F("capabilities"), capabilities);
    w.decreaseDepth();
    w.print("}");
  }

}

/*void CompositeSensor::print(int depth)
{
    JsonSerialWriter w(depth);
    w.println("{");
    w.increaseDepth();
    w.printlnStringObj(F("name"), name.c_str(), ",");
    w.printKey(F("sensors"));
    w.noPrefixPrintln("{");
    w.increaseDepth();
    bool bFirst = true;
    for( Sensor* s : sensors ) {
        if ( bFirst ) {
            bFirst = false;
        } else {
            w.noPrefixPrintln(",");
        }
        s->print(depth+2);
    }
    w.decreaseDepth();
    w.noPrefixPrintln("");
    w.println("}");
    w.decreaseDepth();
    w.print("}");
}*/
