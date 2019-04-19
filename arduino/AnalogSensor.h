#ifndef ANALOG_SENSOR_H
#define ANALOG_SENSOR_H

#include "ArduinoSensor.h"


class AnalogSensor : public ArduinoSensor {
public:

  AnalogSensor(const char* const name, uint8_t sensorPin, uint16_t sampleCnt=1, uint16_t sampleIntervalMs=25)
      : ArduinoSensor(name,sensorPin,sampleCnt,sampleIntervalMs) {
  }

  void setup() override {
    if ( !isInitialized() ) {
      pinMode(sensorPin, INPUT);
      setInitialized(true);
    }
  }

  float getValueImpl() const override {
    return analogRead(sensorPin);
  }
};

#endif
