#ifndef RELAY_SENSOR_H
#define RELAY_SENSOR_H

#include "../automation/Automation.h"
#include "../automation/ArduinoSensor.h"

// 0 for relay/toggle turned off and 1 for on
namespace arduino {
class RelaySensor : public automation::ArduinoSensor {
public:

  bool onValue = HIGH; // some relays are on when signal is set low/false instead of high/true

  RelaySensor(const string& name, uint8_t sensorPin,  bool onValue = HIGH) :
      ArduinoSensor(name, sensorPin), onValue(onValue)
  {
  }

  RTTI_GET_TYPE_IMPL(arduino,RelaySensor)
 
  void setup() override {
    cout << __PRETTY_FUNCTION__ << endl;
    if ( !bInitialized ) {
      pinMode(sensorPin, OUTPUT); // TBD - actual relay in device will call pinMode=OUTPUT too
      bInitialized = true;
    }
  }

  virtual float getValueImpl() {
    return digitalRead(relayPin) == onValue;
    //bitRead(PORTD,relayPin); // uno and mega behave different when reading a write mode digital port
  }

};
}
#endif