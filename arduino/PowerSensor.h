#ifndef ARDUINO_POWER_SENSOR_H
#define ARDUINO_POWER_SENSOR_H

#include "ArduinoSensor.h"
#include "VoltageSensor.h"
#include "CurrentSensor.h"

namespace arduino {

  typedef unsigned char PowerSensorMember;

  const PowerSensorMember POWER_METER_MEMBER_INVALID = -1, POWER_METER_MEMBER_VCC = 0, POWER_METER_MEMBER_R1 = 1, POWER_METER_MEMBER_R2 = 2;

  class PowerSensor : public Sensor {
  public:
    RTTI_GET_TYPE_IMPL(arduino,PowerSensor)
  
    VoltageSensor *pVoltageSensor;
    CurrentSensor *pCurrentSensor;

    PowerSensor(const char *const name, VoltageSensor *pVoltageSensor, CurrentSensor *pCurrentSensor) :
        Sensor(name),
        pVoltageSensor(pVoltageSensor),
        pCurrentSensor(pCurrentSensor) {
          setCanSample(false); 
    }

    float getValueImpl() const override {
      float watts = pVoltageSensor->getValue() * pCurrentSensor->getValue();
      return watts;
    }

    void setup() override {
      if ( !isInitialized() ) {
        pVoltageSensor->setup();
        pCurrentSensor->setup();
        setInitialized(true);
      }
    }

    void printVerboseExtra(JsonStreamWriter& w) const override {
      pVoltageSensor->printlnObj(w,F("voltage"),",",false);
      pCurrentSensor->printlnObj(w,F("current"),",",false);
    }
  };

}
#endif
