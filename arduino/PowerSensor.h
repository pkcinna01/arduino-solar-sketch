#ifndef ARDUINO_POWER_SENSOR_H
#define ARDUINO_POWER_SENSOR_H

#include "ArduinoSensor.h"
#include "VoltageSensor.h"
#include "CurrentSensor.h"

namespace arduino {

  typedef unsigned char PowerSensorMember;

  const PowerSensorMember POWER_METER_MEMBER_INVALID = -1, POWER_METER_MEMBER_VCC = 0, POWER_METER_MEMBER_R1 = 1, POWER_METER_MEMBER_R2 = 2;

  class PowerSensor : public ArduinoSensor {
  public:
    RTTI_GET_TYPE_IMPL(arduino,PowerSensor)
  
    VoltageSensor *pVoltageSensor;
    CurrentSensor *pCurrentSensor;

    PowerSensor(const char *const name, VoltageSensor *pVoltageSensor, CurrentSensor *pCurrentSensor) :
        ArduinoSensor(name, 0),
        pVoltageSensor(pVoltageSensor),
        pCurrentSensor(pCurrentSensor) {
    }

    float getValueImpl() const override {
      float watts = pVoltageSensor->getValue() * pCurrentSensor->getValue();
      return watts;
    }

    void setup() override {
      if ( !bInitialized ) {
        pVoltageSensor->setup();
        pCurrentSensor->setup();
        bInitialized = true;
      }
    }

    void print(JsonStreamWriter& w, bool bVerbose, bool bIncludePrefix) const override {
      if ( bIncludePrefix ) w.println("{"); else w.noPrefixPrintln("{");
 
      w.increaseDepth();
      w.printlnStringObj(F("name"), name, ",");
      w.printlnStringObj(F("id"), id, ",");
      if ( bVerbose ) {
        w.printKey(F("voltage"));
        pVoltageSensor->print(w,bVerbose,false);
        w.noPrefixPrintln(",");
        w.printKey(F("current"));
        pCurrentSensor->print(w,bVerbose,false);
        w.noPrefixPrintln(",");
      }
      w.printlnNumberObj(F("value"), getValue());
      w.decreaseDepth();
      w.print("}");
    }
  };

}
#endif
