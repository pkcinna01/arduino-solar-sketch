#ifndef ARDUINO_COOLING_FAN_H
#define ARDUINO_COOLING_FAN_H

#include "../automation/Automation.h"
#include "../automation/sensor/Sensor.h"
#include "../automation/device/CoolingFan.h"

namespace arduino {

  class CoolingFan : public automation::CoolingFan {
  public:

    RTTI_GET_TYPE_IMPL(arduino,CoolingFan)
   
    int relayPin;
    bool onValue;

    CoolingFan(const string &name, int relayPin, automation::Sensor& tempSensor, float onTemp, float offTemp, bool onValue=true, unsigned int minDurationMs=0) :
        automation::CoolingFan(name,tempSensor,onTemp,offTemp,minDurationMs),
        relayPin(relayPin),
        onValue(onValue)
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
      bool rtn = digitalRead(relayPin) == onValue;
      return rtn;
    }

    void setOn(bool bOn) override {
      digitalWrite(relayPin,bOn?onValue:!onValue);
    }

    void printVerboseExtra(int depth) override {
      JsonSerialWriter w(depth);
      w.noPrefixPrintln(",");
      w.printlnNumberObj(F("relayPin"),relayPin,",");
      w.printlnNumberObj(F("onValue"),onValue,",");
      w.printlnNumberObj(F("onTemp"),minTemp.pThreshold->getValue() + minTemp.getPassMargin(),",");
      w.printlnNumberObj(F("offTemp"),minTemp.pThreshold->getValue() - minTemp.getFailMargin(),",");
      w.printlnNumberObj(F("minDurationMs"),minTemp.getFailDelayMs(),",");
      w.printlnNumberObj(F("currentTemp"),tempSensor.getValue(),"");
    }
  };

}
#endif