
#include "../automation/sensor/Sensor.h"
#include "../automation/sensor/CompositeSensor.h"

#include "JsonWriter.h"

namespace automation {

void Sensor::print(int depth)
{
    float value = getValue();
    JsonSerialWriter w(depth);
    w.println("{");
    w.increaseDepth();
    w.printlnStringObj(F("name"),name.c_str(),",");
    w.printlnNumberObj(F("value"),value);
    w.decreaseDepth();
    w.print("}");
  }
}

void CompositeSensor::printVerbose(int depth)
{
    JsonSerialWriter w(depth);
    w.println("{");
    w.increaseDepth();
    w.printlnStringObj(F("name"), name.c_str(), ",");
    w.printlnNumberObj(F("value"), getValue(), ",");
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
        s->printVerbose(depth+2);
    }
    w.decreaseDepth();
    w.noPrefixPrintln("");
    w.println("}");
    w.decreaseDepth();
    w.print("}");
}
