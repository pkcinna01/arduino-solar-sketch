#ifndef AUTOMATION_VALUEDURATION_CONSTRAINT_H
#define AUTOMATION_VALUEDURATION_CONSTRAINT_H

#include "Constraint.h"
#include "../capability/Capability.h"
#include <sstream>
#include <math.h>

namespace automation {

class TransitionDurationConstraint : public Constraint {
  public:
    unsigned long minIntervalMs;

  TransitionDurationConstraint(unsigned int minIntervalMs, Capability* pCapability, double originValue, double destinationValue, double defaultValue = NAN) :
        minIntervalMs(minIntervalMs), pCapability(pCapability), originValue(originValue), destinationValue(destinationValue){
      lastValue = isnan(defaultValue) ? originValue : defaultValue;
    }

    // don't check for simultaneous events if doing a bulk synchronize of state
    bool isSynchronizable() override {
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

    string getTitle() override {
      stringstream ss;
      ss << "TransitionDuration(" << minIntervalMs << "," << pCapability->getTitle() << " " << originValue << "-->" << destinationValue << ")";
      return ss.str();
    }

  protected:
    unsigned long stateStartTimeMs = 0;
    Capability* pCapability;
    double originValue, destinationValue, lastValue;
  };

}
#endif
