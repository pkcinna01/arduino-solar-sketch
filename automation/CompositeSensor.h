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
      //cout << __PRETTY_FUNCTION__ << endl;
      for( Sensor* s : sensors ) s->setup();
      //cout << __PRETTY_FUNCTION__ << " complete" << endl;
    }

    virtual float getValue() const {
      //cout << __PRETTY_FUNCTION__ << " begin" << endl;
      bool rtn = getValueFn(sensors);
      //cout << __PRETTY_FUNCTION__ << " rtn=" << rtn << endl;
      return rtn;
    }

    virtual void print(int depth = 0);

    friend std::ostream &operator<<(std::ostream &os, const CompositeSensor &s) {
      os << "CompositeSensor{ strType: " << s.name << ", value: " << s.getValue() << "}";
      return os;
    }
  };


}

#endif
