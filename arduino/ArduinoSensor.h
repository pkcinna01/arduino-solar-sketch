#ifndef ARDUINO_SENSOR_H
#define ARDUINO_SENSOR_H

#include "Arduino.h"

#include "../automation/Automation.h"
#include "../automation/Sensor.h"
#include "Status.h"

using namespace arduino;

class ArduinoSensor : public automation::Sensor {
public:

  uint8_t sensorPin;
  uint16_t sampleCnt;
  uint16_t sampleIntervalMs;

  mutable Status status;

  ArduinoSensor(const char* const name, uint8_t sensorPin, uint16_t sampleCnt=1, uint16_t sampleIntervalMs=25)
      : Sensor(name), sensorPin(sensorPin), sampleCnt(sampleCnt), sampleIntervalMs(sampleIntervalMs)
  {
  }

  float getValue() const override {
    status.reset();
    return sample(this, &ArduinoSensor::getValueImpl, sampleCnt, sampleIntervalMs);
  }

  virtual float getValueImpl() const = 0;

  void print(int depth) override {
    float value = getValue();
    JsonSerialWriter w(depth);
    w.println("{");
    w.increaseDepth();
    w.printlnStringObj(F("name"),name.c_str(),",");
    if ( status.code == 0 ) {
      w.printlnNumberObj(F("value"),value);
    } else {
      w.printlnNumberObj(F("value"),value,",");
      w.printKey(F("status"));
      w.noPrefixPrintln("");
      status.print(depth+1);
    }
    w.decreaseDepth();
    w.print("}");
  }

};

#endif