#ifndef ARDUINO_SENSOR_H
#define ARDUINO_SENSOR_H

#include "Arduino.h"

#include "../automation/Automation.h"
#include "../automation/sensor/Sensor.h"
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

  virtual bool setAttribute(const char* pszKey, const char* pszVal, ostream* pRespStream = nullptr){
    if ( Sensor::setAttribute(pszKey,pszVal,pRespStream) ) {
      return true;
    } else {
      if ( !strcasecmp_P(pszKey, PSTR("sensorPin")) ) {
        sensorPin = atoi(pszVal);
      } else if ( !strcasecmp_P(pszKey, PSTR("sampleCnt")) ) {
        sampleCnt = atoi(pszVal);
      } else if ( !strcasecmp_P(pszKey, PSTR("sampleIntervalMs")) ) {
        sampleIntervalMs = atoi(pszVal);
      } else {
        return false;
      }
      if (pRespStream) {
        (*pRespStream) << "'" << name << "' " << pszKey << "=" << pszVal;
      }
      return true;
    }
  }
    
  void print(int depth) override {
    bool bVerbose = false;
    print(depth,bVerbose);
  }

  void printVerbose(int depth) override {
    bool bVerbose = true;
    print(depth,bVerbose);
  }

  protected:

  void print(int depth, bool bVerbose) {
    float value = getValue();
    JsonSerialWriter w(depth);
    w.println("{");
    w.increaseDepth();
    w.printlnStringObj(F("name"),name.c_str(),",");
    if ( bVerbose ) {
      w.printlnNumberObj(F("sensorPin"),sensorPin);
      w.printlnNumberObj(F("sampleCnt"),sampleCnt);
      w.printlnNumberObj(F("sampleIntervalMs"),sampleIntervalMs);
      w.printlnStringObj(F("type"),getType().c_str());
    }
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