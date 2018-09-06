#ifndef ARDUINO_POWERSWITCH_H
#define ARDUINO_POWERSWITCH_H

#include "../automation/Automation.h"
#include "../automation/Sensor.h"
#include "../automation/device/PowerSwitch.h"

namespace arduino {

  class PowerSwitch : public automation::PowerSwitch {
  public:

    int relayPin;
    bool onValue;

    PowerSwitch(const string &name, int relayPin, bool onValue=true) :
        automation::PowerSwitch(name), relayPin(relayPin), onValue(onValue)
    {
    }

    void setup() override {
      if ( !bInitialized ) {
        pinMode(relayPin, OUTPUT);
        setOn(false);
        bInitialized = true;
      }
    }

    bool isOn() const override {
      //cout << __PRETTY_FUNCTION__ << endl;
      bool rtn = digitalRead(relayPin) == onValue;
      //cout << __PRETTY_FUNCTION__ << "=" << rtn << endl;
      return rtn;
    }

    void setOn(bool bOn) override {
      //cout << __PRETTY_FUNCTION__ << " bOn=" << bOn << endl;
      digitalWrite(relayPin,bOn?onValue:!onValue);
    }

  };

}
#endif