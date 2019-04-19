#ifndef AUTOMATION_TOGGLESENSOR_H
#define AUTOMATION_TOGGLESENSOR_H

#include "../Automation.h"
#include "Sensor.h"
#include "../capability/Toggle.h"

#include <string>
#include <vector>

using namespace std;

namespace automation {

  class ToggleSensor : public Sensor {

  public:
    RTTI_GET_TYPE_IMPL(automation,ToggleSensor)

    Toggle* pToggle;

    ToggleSensor(Toggle* pToggle) :
        Sensor(pToggle->getDeviceName()),
        pToggle(pToggle)
    {
      setCanSample(false); 
    }

    float getValueImpl() const override {
      return pToggle->asBoolean();
    }

    friend std::ostream &operator<<(std::ostream &os, const ToggleSensor &s) {
      os << F("ToggleSensor{ type: ") << s.name << F(", value: ") << s.getValue() << "}";
      return os;
    }
  };

}

#endif
