#ifndef AUTOMATION_SIMULTANEOUS_CONSTRAINT_H
#define AUTOMATION_SIMULTANEOUS_CONSTRAINT_H

#include "Constraint.h"
#include "../capability/Capability.h"

#include <algorithm>

namespace automation {

class SimultaneousConstraint : public Constraint, public Capability::CapabilityListener {
  public:
    RTTI_GET_TYPE_IMPL(automation,Simultaneous)
    
    unsigned long maxIntervalMs;

    SimultaneousConstraint(unsigned int maxIntervalMs, Capability* pCapability, double targetValue=1.0) :
      maxIntervalMs(maxIntervalMs), pCapability(pCapability), targetValue(targetValue){
    }

    // don't check for simultaneous events if doing a bulk synchronize of state
    bool isSynchronizable() const override {
      return false;
    }

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

    string getTitle() const override {
        stringstream ss;
        string owner = pCapability->getOwnerName();
        ss << getType() << "(" << owner;
        size_t totalLength = owner.length();
        for ( auto c : capabilityGroup ) {
            owner = c->getOwnerName();
            const size_t maxTitleLen = 60;
            if ( totalLength + owner.length() > maxTitleLen ) {
                if ( totalLength < maxTitleLen ) {
                    ss << owner.substr(0,maxTitleLen-totalLength);
                }
                ss << "...";
                break;
            } else {
                totalLength += owner.length();
                ss << ',' << owner;
            }
        }
        ss << ")";
        return ss.str();
    }

    // add instance of this class as listener to a capability if it is in group of things to be throttled
    // Example:
    // Toggle t1, t2, t3; // make sure these
    // SimultaneousConstraint simultaneousConstraint(60*1000);
    // t1.addListener(simultaneousConstraint);
    // t2.addListener(simultaneousConstraint);
    // t3.addListener(simultaneousConstraint);
    //
    void valueSet(const Capability* pCapability, double newVal, double oldVal) override {
      if ( newVal != oldVal ) {
        return; // capability (ex: toggle) was set but not changed so should not impact simultaneity
      } else if ( automation::bSynchronizing ) {
        return;
      } else if ( pCapability == this->pCapability ) {
        return; // would not make sense to check if a capability is simultaneous with itself
      } else if ( newVal == targetValue ) {
        lastPassTimeMs = millisecs();
        pLastPassCapability = pCapability;
      }
    }

    virtual SimultaneousConstraint& listen( Capability* pCapability ) {
      pCapability->addListener(this);
      capabilityGroup.push_back(pCapability);
      return *this;
    }

    static void connectListeners( std::initializer_list<SimultaneousConstraint*> group) {
      for ( auto pConstraint : group ) {
        for ( auto pCapabilityOwner : group ) {
          if (pConstraint!=pCapabilityOwner) {
            pConstraint->listen(pCapabilityOwner->pCapability);
          }
        }
      }
    }

    virtual void printVerboseExtra(json::JsonStreamWriter& w) const override {
      w.printlnNumberObj(F("maxIntervalMs"),maxIntervalMs,",");
      w.printlnNumberObj(F("remainingMs"), std::max((float)0,(float)maxIntervalMs-(float)(millisecs()-lastPassTimeMs)),",");
      w.printKey(F("capabilityIds"));
      w + F(" [");
      bool bFirst = true;
      for( auto pCap : capabilityGroup){
        if ( bFirst ) {
          bFirst = false;
        } else {
          w + F(",");
        }
        w + (unsigned int) pCap->id;
      }
      w.noPrefixPrintln(F("],"));
    }

  protected:
    unsigned long lastPassTimeMs = 0;
    Capability* pCapability;
    const Capability* pLastPassCapability = nullptr;
    vector<Capability*> capabilityGroup;
    double targetValue; // set to 1 if check for simultaneous toggle ON and set to 0 for toggle OFF
  };

}
#endif
