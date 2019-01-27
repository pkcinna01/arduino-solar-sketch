#ifndef AUTOMATION_SENSOR_H
#define AUTOMATION_SENSOR_H

#include "../Automation.h"
#include "../AttributeContainer.h"

#include <string>
#include <vector>

namespace automation {


  template<typename ValueT>
  class ValueHolder {
  public:
    virtual ValueT getValue() const = 0;
  };

  class Sensor : public ValueHolder<float>, public NamedContainer {
  public:    
    RTTI_GET_TYPE_DECL;
    //GET_ID_DECL;

    Sensor(const std::string& name) : NamedContainer(name)
    {
    }

    virtual void setup()
    {
      bInitialized = true;
    }
  
    virtual void print(json::JsonStreamWriter& w, bool bVerbose=false, bool bIncludePrefix=true) const override;

    template<typename ObjectPtr,typename MethodPtr>
    static float sample(ObjectPtr pObj, MethodPtr pMethod, unsigned int cnt = 5, unsigned int intervalMs = 50)
    {
      float sum = 0;
      if ( cnt == 0 ) {
        cnt = 1;
      }
      for (int i = 0; i < cnt; i++)
      {
        sum += (pObj->*pMethod)();
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
      os << F("\"") << s.name << F("\": ") << s.getValue();
      return os;
    }

  protected:
    bool bInitialized = false;
  };


  class SensorFn : public Sensor {
  public:
    RTTI_GET_TYPE_IMPL(automation::sensor,SensorFn)

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


  // Use another sensor as the source for data but transform it based on a custom function
  class TransformSensor : public Sensor {
  public:
    Sensor& sourceSensor;
    float(*transformFn)(float);
    std::string transformName;

    TransformSensor(const std::string& transformName, Sensor& sourceSensor, float(*transformFn)(float)):
      Sensor( transformName + "(" + sourceSensor.name + ")"),
      transformName(transformName),
      sourceSensor(sourceSensor),
      transformFn(transformFn) {
    }
    virtual float getValue() const
    {
      return transformFn(sourceSensor.getValue());
    }

  };

  class Sensors : public NamedItemVector<Sensor*> {
  public:
    Sensors(){}
    Sensors( vector<Sensor*>& sensors ) : NamedItemVector<Sensor*>(sensors) {}
    Sensors( vector<Sensor*> sensors ) : NamedItemVector<Sensor*>(sensors) {}
  };

}

#endif
