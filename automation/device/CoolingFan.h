#ifndef ARDUINO_SOLAR_SKETCH_COOLINGFAN_H
#define ARDUINO_SOLAR_SKETCH_COOLINGFAN_H

#include "../sensor/Sensor.h"
#include "PowerSwitch.h"
#include "../constraint/ValueConstraint.h"

using namespace std;

namespace automation {

  class CoolingFan: public PowerSwitch {

  public:

    Sensor& tempSensor;
    AtLeast<float,Sensor&> minTemp; // any temp greater than this min will PASS (turn fan on)

    CoolingFan(const string &name, Sensor& tempSensor, float onTemp, float offTemp, unsigned int minDurationMs=0) :
        PowerSwitch(name),
        tempSensor(tempSensor),
        minTemp(onTemp,tempSensor) {
      minTemp.setFailMargin(onTemp-offTemp).setFailDelayMs(minDurationMs);
      pConstraint = &minTemp;
    }

    virtual SetCode setAttribute(const char* pszKey, const char* pszVal, ostream* pRespStream) override {
      string strResultValue;
      SetCode rtn = PowerSwitch::setAttribute(pszKey,pszVal,pRespStream);
      if ( rtn == SetCode::Ignored ) {
        if ( !strcasecmp_P(pszKey,PSTR("onTemp")) ) {          
          float offTemp = getOffTemp(); 
          rtn = minTemp.setAttribute( RVSTR("threshold"), pszVal );
          if ( rtn == SetCode::OK ) {
            setOffTemp(offTemp); // off temp is stored relative to on temp so adjust it
            strResultValue = text::asString(minTemp.pThreshold->getValue());
            minTemp.setPassMargin(0); // should be 0 already
          }
        } else if ( !strcasecmp_P(pszKey,PSTR("offTemp")) ) {
          float fOffTemp = atof(pszVal);
          setOffTemp(fOffTemp);
          strResultValue = text::asString(minTemp.pThreshold->getValue()-minTemp.getFailMargin());
          rtn = SetCode::OK;
        } else if ( !strcasecmp_P(pszKey,PSTR("minDurationMs")) ) { 
          unsigned long durationMs = atol(pszVal);
          minTemp.setFailDelayMs(durationMs); // fan will run at least this duration
          strResultValue = text::asString(minTemp.getFailDelayMs());
          rtn = SetCode::OK;
        }
        if (pRespStream && rtn == SetCode::OK ) {
          (*pRespStream) << pszKey << "=" << strResultValue;
        }
      }
      return rtn;
    }

    void setOffTemp(float offTemp) {
      minTemp.setFailMargin( minTemp.pThreshold->getValue() - offTemp);
    }

    float getOffTemp() const { 
      return minTemp.pThreshold->getValue() - minTemp.getFailMargin();
    }

    void printVerboseExtra(JsonStreamWriter& w) const {
      automation::PowerSwitch::printVerboseExtra(w);
      w.printlnNumberObj(F("onTemp"),minTemp.pThreshold->getValue() + minTemp.getPassMargin(),",");
      w.printlnNumberObj(F("offTemp"),getOffTemp(),",");
      w.printlnNumberObj(F("minDurationMs"),minTemp.getFailDelayMs(),",");
      w.printlnNumberObj(F("currentTemp"),tempSensor.getValue(),",");
    }

  };

}
#endif //ARDUINO_SOLAR_SKETCH_COOLINGFAN_H
