#ifndef AUTOMATION_SENSOR_H
#define AUTOMATION_SENSOR_H

#include "Automation.h"

#include <string>
#include <functional>
#include <iostream>
#include <vector>

namespace automation {

  class Sensor {
  public:

    std::string name;

    Sensor(const std::string& name) : name(name)
    {
    }

    virtual void setup()
    {
      bInitialized = true;
    }

    virtual float getValue() const = 0;

    virtual void print(int depth = 0);

    virtual void printVerbose(int depth = 0 ) { print(depth); }

    virtual const string& getTitle() const {
      return name;
    }

    template<typename ObjectPtr,typename MethodPtr>
    static float sample(ObjectPtr obj, MethodPtr method, unsigned int cnt = 5, unsigned int intervalMs = 50)
    {
      float sum = 0;
      if ( cnt == 0 ) {
        cnt = 1;
      }
      for (int i = 0; i < cnt; i++)
      {
        sum += (obj->*method)();
        if ( cnt > 1 )
        {
          automation::sleep(intervalMs);
        }
      }
      return sum/cnt;
    }

    static bool compareValues(const Sensor* lhs, const Sensor* rhs);

    static float average(const vector<Sensor*>& sensors);
    static float minimum(const vector<Sensor*>& sensors);
    static float maximum(const vector<Sensor*>& sensors);
    static float delta(const vector<Sensor*>& sensors);

    friend std::ostream &operator<<(std::ostream &os, const Sensor &s) {
      os << "\"Sensor\": { name: \"" << s.name << "\", value: " << s.getValue() << " }";
      return os;
    }

  protected:
    bool bInitialized = false;
  };

  class SensorFn : public Sensor {
  public:

    //const std::function<float()> getValueImpl;
    float (*getValueImpl)(); // Arduino does not support function<> template

    SensorFn(const std::string& name, float (*getValueImpl)())
        : Sensor(name), getValueImpl(getValueImpl)
    {
    }

    virtual float getValue() const
    {
      return getValueImpl();
    }

    virtual float operator()() const {
      return getValue();
    }
  };

  class ScaledSensor : public Sensor {
  public:
    Sensor& sourceSensor;
    float(*scaleFn)(float);

    ScaledSensor(Sensor& sourceSensor, float(*scaleFn)(float)):
      Sensor( string("SCALED(") + sourceSensor.name + ")"),
      sourceSensor(sourceSensor),
      scaleFn(scaleFn) {
    }
    virtual float getValue() const
    {
      return scaleFn(sourceSensor.getValue());
    }

  };

  class Sensors : public AutomationVector<Sensor*> {
  public:
    Sensors(){}
    Sensors( vector<Sensor*>& sensors ) : AutomationVector<Sensor*>(sensors) {}
    Sensors( vector<Sensor*> sensors ) : AutomationVector<Sensor*>(sensors) {}
  };

}

#endif
