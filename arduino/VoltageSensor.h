
#ifndef ARDUINO_VOLTAGE_SENSOR_H
#define ARDUINO_VOLTAGE_SENSOR_H

#include "AnalogSensor.h"
#include "JsonWriter.h"

class VoltageSensor : public AnalogSensor {
public:

  float r1;
  float r2; // measured from analog pin to ground
  float vcc;

  VoltageSensor(const char *name, int analogPin, float r1 = 1000000.0, float r2 = 100000.0, float vcc = 5.0) :
      AnalogSensor(name, analogPin, 10 /*sample cnt*/),
      r1(r1),
      r2(r2),
      vcc(vcc) {
  }

  float getValueImpl() const override {
    return readVoltage();
  }

  float readVoltage() const {
    float adc = analogRead(sensorPin);
    float dividerWeight = ((float) (r1 + r2)) / r2;
    return dividerWeight * adc * vcc / 1024.0;
  }

  virtual void print(int depth = 0) {
    print(depth, false);
  }

  virtual void print(int depth, bool sameLine) {
    float assignedVcc = vcc;
    float assignedR1 = r1;
    float assignedR2 = r2;

    JsonSerialWriter w(depth);
    if (sameLine) {
      w.noPrefixPrintln("{");
    } else {
      w.println("{");
    }
    w.increaseDepth();
    w.printlnStringObj(F("name"), name.c_str(), ",");
    w.printlnNumberObj(F("volts"), getValue(), ",");
    w.printlnNumberObj(F("analogPin"), sensorPin, ",");
    w.printlnNumberObj(F("assignedVcc"), assignedVcc, ",");
    w.printlnNumberObj(F("assignedR1"), assignedR1, ",");
    w.printlnNumberObj(F("assignedR2"), assignedR2);
    w.decreaseDepth();
    w.print("}");
  }

};

#endif