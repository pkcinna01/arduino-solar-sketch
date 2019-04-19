#ifndef AUTOMATION_VALUEDURATION_CONSTRAINT_H
#define AUTOMATION_VALUEDURATION_CONSTRAINT_H

#include "Constraint.h"
#include "../capability/Capability.h"
#include <sstream>
#include <math.h>

namespace automation {

class TransitionDurationConstraint : public Constraint {
  public:
    RTTI_GET_TYPE_IMPL(automation,TransitionDuration)
    
    unsigned long minIntervalMs;

  TransitionDurationConstraint(unsigned long minIntervalMs, Capability* pCapability, double originValue, double destinationValue, double defaultValue = NAN) :
        minIntervalMs(minIntervalMs), pCapability(pCapability), originValue(originValue), destinationValue(destinationValue){
      lastValue = isnan(defaultValue) ? originValue : defaultValue;
    }

    // don't check for simultaneous events if doing a bulk synchronize of state
    bool isSynchronizable() const override {
      return false;
    }

    bool checkValue() override {
      double value = pCapability->getValue();
      bool bValuePassedForDuration = true;

      if ( value == destinationValue ) {
        lastValue = value;
        return true; // already have desired value
      }

      unsigned long now = millisecs();

      if ( originValue != lastValue ) {
        stateStartTimeMs = now;
      }
      lastValue = value;
      unsigned long elapsedMs = now - stateStartTimeMs;
      bValuePassedForDuration = elapsedMs >= minIntervalMs;
      return bValuePassedForDuration;
    }

    string getTitle() const override {
      stringstream ss;
      ss << "TransitionDuration(" << minIntervalMs << ',' << pCapability->getTitle() << " " << originValue << F("-->") << destinationValue << ")";
      return ss.str();
    }

    virtual void printVerboseExtra(json::JsonStreamWriter& w) const override {
      w.printlnNumberObj(F("minIntervalMs"),minIntervalMs,",");
      w.printlnNumberObj(F("originValue"),originValue,",");
      w.printlnNumberObj(F("destinationValue"),destinationValue,",");
      w.printlnNumberObj(F("lastValue"),lastValue,",");
      w.printlnNumberObj(F("elapsedMs"),millisecs()-stateStartTimeMs,",");
    }

  protected:
    unsigned long stateStartTimeMs = 0;
    Capability* pCapability;
    double originValue, destinationValue, lastValue;
  };

}
#endif
