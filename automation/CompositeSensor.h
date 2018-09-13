#ifndef AUTOMATION_COMPOSITESENSOR_H
#define AUTOMATION_COMPOSITESENSOR_H

#include "Automation.h"
#include "Sensor.h"

#include <string>
#include <vector>

using namespace std;

namespace automation {

  class CompositeSensor : public Sensor {

  public:

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

    void printVerbose(int depth = 0) override;

    friend std::ostream &operator<<(std::ostream &os, const CompositeSensor &s) {
      os << "CompositeSensor{ strType: " << s.name << ", value: " << s.getValue() << "}";
      return os;
    }
  };


}

#endif
