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

  mutable Status status;

  ArduinoSensor(const char* const name, uint8_t sensorPin, uint16_t sampleCnt=1, uint16_t sampleIntervalMs=35)
      : Sensor(name,sampleCnt,sampleIntervalMs), sensorPin(sensorPin)
  {
  }

  void reset() override { 
    automation::Sensor::reset();
    status.reset(); 
  }

  virtual SetCode setAttribute(const char* pszKey, const char* pszVal, ostream* pRespStream = nullptr){
    SetCode rtn = Sensor::setAttribute(pszKey,pszVal,pRespStream);
    if ( rtn == SetCode::Ignored ) {
      if ( !strcasecmp_P(pszKey, PSTR("sensorPin")) ) {
        sensorPin = atoi(pszVal);
        rtn = SetCode::OK;
      } else if ( !strcasecmp_P(pszKey, PSTR("sampleCnt")) ) {
        sampleCnt = atoi(pszVal);
        rtn = SetCode::OK;
      } else if ( !strcasecmp_P(pszKey, PSTR("sampleIntervalMs")) ) {
        sampleIntervalMs = atoi(pszVal);
        rtn = SetCode::OK;
      }
      if (pRespStream && rtn == SetCode::OK ) {
        (*pRespStream) << "'" << name << "' " << pszKey << "=" << pszVal;
      }
    }
    return rtn;
  }
    
  virtual void print(JsonStreamWriter& w, bool bVerbose, bool bIncludePrefix) const override {
    float value = getValue();
    if ( bIncludePrefix ) w.println("{"); else w.noPrefixPrintln("{");
    w.increaseDepth();
    w.printlnStringObj(F("name"),name,",");
    w.printlnStringObj(F("id"),id,",");
    if ( bVerbose ) {
      w.printlnNumberObj(F("sensorPin"),sensorPin,",");
      w.printlnNumberObj(F("sampleCnt"),sampleCnt,",");
      w.printlnNumberObj(F("sampleIntervalMs"),sampleIntervalMs,",");
      w.printlnStringObj(F("type"),getType(),",");
      printVerboseExtra(w);
    }
    if ( status.code != 0 ) {
      w.printKey(F("status"));
      status.noPrefixPrint(w,bVerbose);
      w.noPrefixPrintln(",");
    }
    w.printlnNumberObj(F("value"),value);
    w.decreaseDepth();
    w.print("}");
  }
};

#endif
