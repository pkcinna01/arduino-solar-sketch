#ifndef AUTOMATION_TOGGLESENSOR_H
#define AUTOMATION_TOGGLESENSOR_H

#include "Automation.h"
#include "Sensor.h"
#include "capability/Toggle.h"

#include <string>
#include <vector>

using namespace std;

namespace automation {

  class ToggleSensor : public Sensor {

  public:

    Toggle* pToggle;

    ToggleSensor(Toggle* pToggle) :
        Sensor(pToggle->getTitle()),
        pToggle(pToggle)
    {
    }

    float getValue() const override {
      return pToggle->asBoolean();
    }

    friend std::ostream &operator<<(std::ostream &os, const ToggleSensor &s) {
      os << "ToggleSensor{ type: " << s.name << ", value: " << s.getValue() << "}";
      return os;
    }
  };

}

#endif
