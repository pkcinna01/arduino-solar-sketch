#ifndef ARDUINO_COOLING_FAN_H
#define ARDUINO_COOLING_FAN_H

#include "../automation/Automation.h"
#include "../automation/sensor/Sensor.h"
#include "../automation/device/CoolingFan.h"

namespace arduino {

  class CoolingFan : public automation::CoolingFan {
  public:

    RTTI_GET_TYPE_IMPL(arduino,CoolingFan)
   
    int relayPin;
    bool relayOnSignal;

    CoolingFan(const string &name, int relayPin, automation::Sensor& tempSensor, float onTemp, float offTemp, bool relayOnSignal=true, unsigned int minDurationMs=0) :
        automation::CoolingFan(name,tempSensor,onTemp,offTemp,minDurationMs),
        relayPin(relayPin),
        relayOnSignal(relayOnSignal)
    {
    }

    void setup() override {
      if ( !bInitialized ) {
        pinMode(relayPin, OUTPUT);
        setOn(false);
        bInitialized = true;
      }
    }

    bool isOn() const override {
      bool rtn = digitalRead(relayPin) == relayOnSignal;
      return rtn;
    }

    void setOn(bool bOn) override {
      digitalWrite(relayPin,bOn?relayOnSignal:!relayOnSignal);
    }

    virtual SetCode setAttribute(const char* pszKey, const char* pszVal, ostream* pRespStream) override {
      string strResultValue;
      SetCode rtn = automation::CoolingFan::setAttribute(pszKey,pszVal,pRespStream);
      if ( rtn == SetCode::Ignored ) {
        if ( !strcasecmp_P(pszKey,PSTR("relayPin")) ) {
          relayPin = atol(pszVal);
          strResultValue = text::asString(relayPin);
          rtn = SetCode::OK;
        } else if ( !strcasecmp_P(pszKey,PSTR("relayOnSignal")) ) {
          if ( !strcasecmp_P(pszVal,PSTR("HIGH")) ) {
            relayOnSignal = true;
            strResultValue = text::boolAsString(relayOnSignal);
            rtn = SetCode::OK;
          } else if ( !strcasecmp_P(pszVal,PSTR("LOW")) ) {
            relayOnSignal = false;
            strResultValue = text::boolAsString(relayOnSignal);
            rtn = SetCode::OK;
          } else {
            rtn = SetCode::Error;
            if (pRespStream) {
              (*pRespStream) << RVSTR("Expected HIGH or LOW but found ") << pszVal;
            }
          }
        }
        if (pRespStream && rtn == SetCode::OK ) {
          (*pRespStream) << pszKey << "=" << strResultValue;
        }
      }
      return rtn;
    }

    void printVerboseExtra(JsonStreamWriter& w) const {
      automation::CoolingFan::printVerboseExtra(w);
      w.printlnNumberObj(F("relayPin"),(int)relayPin,",");
      w.printlnStringObj(F("relayOnSignal"),relayOnSignal?F("HIGH"):F("LOW"),",");
    }
  };

}
#endif