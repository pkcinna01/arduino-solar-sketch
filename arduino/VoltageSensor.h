
#ifndef ARDUINO_VOLTAGE_SENSOR_H
#define ARDUINO_VOLTAGE_SENSOR_H

#include "AnalogSensor.h"
#include "JsonWriter.h"

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
      AnalogSensor(name, analogPin, 5 /*sample cnt*/),
      r1(r1),
      r2(r2),
      maxVccAgeMs(maxVccAgeMs) {
  }

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

  void printVerbose(int depth) override {
    float assignedVcc = vcc;
    float assignedR1 = r1;
    float assignedR2 = r2;

    JsonSerialWriter w(depth);
    w.println("{");
    w.increaseDepth();
    w.printlnStringObj(F("name"), name.c_str(), ",");
    w.printlnNumberObj(F("value"), getValue(), ",");
    w.printlnNumberObj(F("analogPin"), sensorPin, ",");
    w.printlnNumberObj(F("assignedVcc"), assignedVcc, ",");
    w.printlnNumberObj(F("assignedR1"), assignedR1, ",");
    w.printlnNumberObj(F("assignedR2"), assignedR2);
    w.decreaseDepth();
    w.print("}");
  }

};

float VoltageSensor::vcc = 5;
unsigned long VoltageSensor::lastVccReadMs = 0;

#endif