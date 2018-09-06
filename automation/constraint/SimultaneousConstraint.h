#ifndef AUTOMATION_SIMULTANEOUS_CONSTRAINT_H
#define AUTOMATION_SIMULTANEOUS_CONSTRAINT_H

#include "Constraint.h"
#include "../capability/Capability.h"

namespace automation {

class SimultaneousConstraint : public Constraint, public Capability::CapabilityListener {
  public:
    unsigned long maxIntervalMs;

    SimultaneousConstraint(unsigned int maxIntervalMs, Capability* pCapability, double targetValue=1.0) :
      maxIntervalMs(maxIntervalMs), pCapability(pCapability), targetValue(targetValue){
    }

    // don't check for simultaneous events if doing a bulk synchronize of state
    bool isSynchronizable() override {
      return false;
    }

    // considered simultaneous if last non-zero capability result happened within maxIntervalMs millisecs
    bool checkValue() override {
      unsigned long now = millisecs();
      unsigned long elapsedMs = now - lastPassTimeMs;
      bool bLastPassRecent = elapsedMs <= maxIntervalMs;
      if ( pCapability->getValue() != targetValue && (!pLastPassCapability || pLastPassCapability->getValue() == targetValue) && bLastPassRecent) {
        //logBuffer << __PRETTY_FUNCTION__ << "******* last PASS was simultaneous. *******" << endl;
        //logBuffer << "\telapsedMs:" <<elapsedMs << ", maxIntervalMs:" << maxIntervalMs << endl;
        return true;
      }
      return false;
    }

    string getTitle() override {
      string title = "Simultaneous";
      return title;
    }

    // add instance of this class as listener to a capability if it is in group of things to be throttled
    // Example:
    // Toggle t1, t2, t3; // make sure these
    // SimultaneousConstraint simultaneousConstraint(60*1000);
    // t1.addListener(simultaneousConstraint);
    // t2.addListener(simultaneousConstraint);
    // t3.addListener(simultaneousConstraint);
    //
    void valueSet(const Capability* pCapability, double numericValue) override {
      if ( automation::bSynchronizing ) {
        return;
      }
      if ( numericValue == targetValue ) {
        lastPassTimeMs = millisecs();
        pLastPassCapability = pCapability;
      }
    }

    virtual SimultaneousConstraint& listen( Capability* c ) {
      c->addListener(this);
      return *this;
    }

  protected:
    unsigned long lastPassTimeMs = 0;
    Capability* pCapability;
    const Capability* pLastPassCapability = nullptr;
    double targetValue; // set to 1 if check for simultaneous toggle ON and set to 0 for toggle OFF
  };

}
#endif
