#ifndef ARDUINO_SENSOR_H
#define ARDUINO_SENSOR_H

#include "Arduino.h"

#include "../automation/Automation.h"
#include "../automation/Sensor.h"

using namespace arduino;

class ArduinoSensor : public automation::Sensor {
public:

  uint8_t sensorPin;
  uint16_t sampleCnt;
  uint16_t sampleIntervalMs;

  ArduinoSensor(const char* const name, uint8_t sensorPin, uint16_t sampleCnt=1, uint16_t sampleIntervalMs=25)
      : Sensor(name), sensorPin(sensorPin), sampleCnt(sampleCnt), sampleIntervalMs(sampleIntervalMs)
  {
  }

  float getValue() const override {
    return sample(this, &ArduinoSensor::getValueImpl, sampleCnt, sampleIntervalMs);
  }

  virtual float getValueImpl() const = 0;

};

#endif