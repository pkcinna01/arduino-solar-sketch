#include "Sensor.h"
#include "CompositeSensor.h"
#include "../json/JsonStreamWriter.h"

#include <algorithm>

using namespace automation::json;

namespace automation {

  float Sensor::average(const vector<Sensor*>& sensors) {
    float total, sensorCnt = sensors.size();
    for ( Sensor* s : sensors) total += s->getValue();
    return total/sensorCnt;
  }

  bool Sensor::compareValues(const Sensor* lhs, const Sensor* rhs){
    return lhs->getValue() < rhs->getValue();
  }

  float Sensor::minimum(const vector<Sensor*>& sensors) {
    auto it = std::min_element( sensors.begin(), sensors.end(), Sensor::compareValues);
    return it == sensors.end() ? 0 : (*it)->getValue();
  }

  float Sensor::maximum(const vector<Sensor*>& sensors) {
    auto it = std::max_element( sensors.begin(), sensors.end(), Sensor::compareValues);
    return it == sensors.end() ? 0 : (*it)->getValue();
  }

  float Sensor::delta(const vector<Sensor*>& sensors) {
    return sensors[0]->getValue() - sensors[1]->getValue();
  }

  void Sensor::print(JsonStreamWriter& w, bool bVerbose, bool bIncludePrefix) const {
    float value = getValue();
    if ( bIncludePrefix ) w.println("{"); else w.noPrefixPrintln("{");
    w.increaseDepth();
    w.printlnStringObj(F("name"), name, ",");
    w.printlnNumberObj(F("id"), (unsigned long) id, ",");
    if ( bVerbose ) {
      w.printlnStringObj(F("type"), getType(), ",");
      printVerboseExtra(w);
    }
    w.printlnNumberObj(F("value"), value);
    w.decreaseDepth();
    w.print("}");
  }

  void CompositeSensor::print(JsonStreamWriter& w, bool bVerbose, bool bIncludePrefix) const {
    if ( bIncludePrefix ) w.println("{"); else w.noPrefixPrintln("{");
    w.increaseDepth();
    w.printlnStringObj(F("name"), name, ",");
    w.printlnNumberObj(F("id"), (unsigned long) id, ",");
    if ( bVerbose ) {
      w.printlnStringObj(F("type"),getType(),",");
      w.printlnVectorObj(F("sensors"),sensors,",");
      string strValFn;
      if ( getValueFn == Sensor::average ) strValFn = RVSTR("average");
      else if ( getValueFn == Sensor::minimum ) strValFn = RVSTR("minimum");
      else if ( getValueFn == Sensor::maximum ) strValFn = RVSTR("maximum");
      else if ( getValueFn == Sensor::delta ) strValFn = RVSTR("delta");
      else strValFn = RVSTR("custom");
      w.printlnStringObj(F("function"),strValFn,",");
    }
    w.printlnNumberObj(F("value"), getValue());
    w.decreaseDepth();
    w.print("}");
  }

}

