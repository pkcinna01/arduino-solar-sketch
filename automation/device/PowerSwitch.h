#ifndef AUTOMATION_POWERSWITCH_H
#define AUTOMATION_POWERSWITCH_H

#include "Device.h"
#include "../capability/Toggle.h"
#include "../ToggleSensor.h"

#include <functional>

using namespace std;

namespace automation {
    class PowerSwitch : public Device {

    public:

      struct PowerSwitchToggle : automation::Toggle {
        PowerSwitchToggle(PowerSwitch* pPowerSwitch) : Toggle(pPowerSwitch), pPowerSwitch(pPowerSwitch) {};
        PowerSwitch* pPowerSwitch;
        double getValueImpl() const override {
          //cout << __PRETTY_FUNCTION__ << endl;
          return (double) pPowerSwitch->isOn();
        }
        void setValueImpl(double val) override {
          //cout << __PRETTY_FUNCTION__ << endl;
          pPowerSwitch->setOn(val!=0);
        }
      } toggle;

      ToggleSensor toggleSensor; // allow the switch state to be seen as a sensor

      PowerSwitch(const string &name) : Device(name), toggle(this), toggleSensor(&toggle) {
        capabilities.push_back(&toggle);
      }

      virtual bool isOn() const = 0;
      virtual void setOn(bool bOn) = 0;

      virtual void constraintResultChanged(bool bConstraintResult) {
        //cout << __PRETTY_FUNCTION__ << " bConstraintResult: " << bConstraintResult << endl;
        toggle.setValue(bConstraintResult);
      }
    };

}
#endif //AUTOMATION_POWERSWITCH_H
