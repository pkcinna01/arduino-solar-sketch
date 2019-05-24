#ifndef ARDUINO_POWERSWITCH_H
#define ARDUINO_POWERSWITCH_H

#include "../automation/Automation.h"
#include "../automation/sensor/Sensor.h"
#include "../automation/device/PowerSwitch.h"

namespace arduino {

  class PowerSwitch : public automation::PowerSwitch {
  public:

    RTTI_GET_TYPE_IMPL(arduino,PowerSwitch)

    unsigned char relayPin;
    bool relayOnSignal;

    PowerSwitch(const string &name, int relayPin, bool relayOnSignal=true) :
        automation::PowerSwitch(name), relayPin(relayPin), relayOnSignal(relayOnSignal)
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
      bool rtn = digitalRead(relayPin) == relayOnSignal;
      //cout << __PRETTY_FUNCTION__ << "=" << rtn << endl;
      return rtn;
    }

    void setOn(bool bOn) override {
      //cout << __PRETTY_FUNCTION__ << " bOn=" << bOn << endl;
      digitalWrite(relayPin,bOn?relayOnSignal:!relayOnSignal);
    }

    void printVerboseExtra(JsonStreamWriter& w) const {
      automation::PowerSwitch::printVerboseExtra(w);
      w.printlnNumberObj(F("relayPin"),(int)relayPin,",");
      w.printlnStringObj(F("relayOnSignal"),relayOnSignal?F("HIGH"):F("LOW"),",");
    }

  };

}
#endif