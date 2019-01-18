
#ifndef ARDUINO_LIGHT_SENSOR_H
#define ARDUINO_LIGHT_SENSOR_H

// default implementation is photoresistor

#include "AnalogSensor.h"

namespace arduino {
class LightSensor : public AnalogSensor {
  public:

  LightSensor(const char* const name,
             int sensorPin,
             float balanceResistance = 10000.0):
    AnalogSensor(name,sensorPin,10,25),
    balanceResistance(balanceResistance)
  {
  }

  RTTI_GET_TYPE_IMPL(arduino,LightSensor)
  
  float getValueImpl() const override {
    int percent = 100.0 * analogRead(sensorPin)/1023.0;
    //float rThermistor = balanceResistance * ( (1023.0 / pinVoltage) - 1);
    //return rThermistor;
    // Vout/Vin = R2/(R1+R2)
    return percent;
  }

protected:
  float balanceResistance;

};
}
#endif