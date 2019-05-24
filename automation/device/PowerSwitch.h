#ifndef AUTOMATION_POWERSWITCH_H
#define AUTOMATION_POWERSWITCH_H

#include "Device.h"
#include "../capability/Toggle.h"
#include "../sensor/ToggleSensor.h"

#include <functional>

using namespace std;

namespace automation
{
class PowerSwitch : public Device
{

public:
  float requiredWatts;

  struct PowerSwitchToggle : automation::Toggle
  {
    PowerSwitchToggle(PowerSwitch *pPowerSwitch) : Toggle(pPowerSwitch), pPowerSwitch(pPowerSwitch){};
    PowerSwitch *pPowerSwitch;
    float getValueImpl() const override
    {
      //cout << __PRETTY_FUNCTION__ << endl;
      return (float)pPowerSwitch->isOn();
    }
    bool setValueImpl(float val) override
    {
      //cout << __PRETTY_FUNCTION__ << "=" << val << endl;
      pPowerSwitch->setOn(val != 0);
      return true;
    }
  } toggle;

  ToggleSensor toggleSensor; // allow the switch state to be seen as a sensor

  PowerSwitch(const string &name, float requiredWatts = 0) : Device(name), requiredWatts(requiredWatts), toggle(this), toggleSensor(&toggle)
  {
    capabilities.push_back(&toggle);
  }

  virtual bool isOn() const = 0;
  virtual void setOn(bool bOn) = 0;

  virtual void constraintResultChanged(bool bConstraintResult)
  {
    //cout << __PRETTY_FUNCTION__ << " bConstraintResult: " << bConstraintResult << endl;
    toggle.setValue(bConstraintResult);
  }

  virtual SetCode setAttribute(const char* pszKey, const char* pszVal, ostream* pRespStream = nullptr) override {
    SetCode rtn = Device::setAttribute(pszKey,pszVal,pRespStream);
    if ( rtn == SetCode::Ignored ) {
      if ( !strcasecmp_P(pszKey,PSTR("ON")) ) {
        if ( pConstraint && !pConstraint->isRemoteCompatible() ) {
          if ( pRespStream ) {
            *pRespStream << RVSTR("Constraint mode not remote compatible: ") << Constraint::modeToString(pConstraint->mode);
          }
          rtn = SetCode::Error;
        } else {
          bool bNewOn = text::parseBool(pszVal);
          pConstraint->overrideTestResult(bNewOn);
          //cout << __PRETTY_FUNCTION__ << " Setting toggle value to " << bNewOn << endl;
          toggle.setValue(bNewOn);
          //cout << __PRETTY_FUNCTION__ << " Set toggle complete." << endl;
          rtn = SetCode::OK;        
        }
      }
    }
    return rtn;
  }

  void printVerboseExtra(json::JsonStreamWriter& w) const {
      automation::Device::printVerboseExtra(w);
      w.printlnBoolObj(F("on"),isOn(),",");
    }

};
} // namespace automation
#endif //AUTOMATION_POWERSWITCH_H
