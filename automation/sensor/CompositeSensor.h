#ifndef AUTOMATION_COMPOSITESENSOR_H
#define AUTOMATION_COMPOSITESENSOR_H

#include "../Automation.h"
#include "Sensor.h"

#include <string>
#include <vector>

using namespace std;

namespace automation {

  class CompositeSensor : public Sensor {

  public:

    RTTI_GET_TYPE_IMPL(automation,CompositeSensor)

    const vector<Sensor*>& sensors;
    float(*getValueFn)(const vector<Sensor*>&);

    CompositeSensor(const string& name, const vector<Sensor*>& sensors, float(*getValueFn)(const vector<Sensor*>&)=Sensor::average) :
        Sensor(name),
        sensors(sensors),
        getValueFn(getValueFn)
    {
    }

    virtual void setup()
    {
      for( Sensor* s : sensors ) s->setup();
    }

    virtual float getValue() const
    {
      return getValueFn(sensors);
    }

    void print(json::JsonStreamWriter& w, bool bVerbose, bool bIncludePrefix) const override;

    friend std::ostream &operator<<(std::ostream &os, const CompositeSensor &s) {
      os << F("CompositeSensor{ strType: ") << s.name << F(", value: ") << s.getValue() << "}";
      return os;
    }
  };


}

#endif
