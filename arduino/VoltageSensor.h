
#ifndef ARDUINO_VOLTAGE_SENSOR_H
#define ARDUINO_VOLTAGE_SENSOR_H

#include "AnalogSensor.h"
#include "../automation/json/JsonStreamWriter.h"

using namespace automation::json;

namespace arduino {
class VoltageSensor : public AnalogSensor {
public:

  float r1;
  float r2; // analog pin to ground
  unsigned long maxVccAgeMs;

  static float vcc;
  static unsigned long lastVccReadMs;

  static void updateVcc(float maxMs = 0) {
    unsigned long nowMs = automation::millisecs();
    if ( (nowMs - lastVccReadMs) > maxMs) {
      vcc = arduino::readVcc() / 1000.0;
      lastVccReadMs = nowMs;
      //cout << __PRETTY_FUNCTION__ << " Vcc updated to " << vcc << endl;
    }
  }

  VoltageSensor(const char *name, int analogPin, float r1 = 1000000.0, float r2 = 100000.0, float maxVccAgeMs = 60000) :
      AnalogSensor(name, analogPin, 10 /*sample cnt*/),
      r1(r1),
      r2(r2),
      maxVccAgeMs(maxVccAgeMs) {
  }

  RTTI_GET_TYPE_IMPL(arduino,VoltageSensor)
  
  float getValueImpl() const override {
    return readVoltage();
  }

  float readVoltage() const {
    updateVcc(maxVccAgeMs);
    float vr2 = VoltageSensor::vcc * analogRead(sensorPin) / 1023.0;
    float dividerWeight = ((float) (r1 + r2)) / r2;
    float vin = dividerWeight * vr2;
    return vin;
  }

  void printVerboseExtra(JsonStreamWriter& w) const override {
    w.printlnNumberObj(F("vcc"), vcc, ",");
    w.printlnNumberObj(F("r1"), r1, ",");
    w.printlnNumberObj(F("r2"), r2, ",");
  }

};

float VoltageSensor::vcc = 5;
unsigned long VoltageSensor::lastVccReadMs = 0;
}
#endif