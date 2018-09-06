#ifndef AUTOMATION_DEVICE_H
#define AUTOMATION_DEVICE_H

#include <vector>
#include <string>
#include <memory>
#include "../constraint/Constraint.h"

using namespace std;

namespace automation {

  class Capability;

  class Device {
  public:

    string name;

    Device(const string &name) :
        name(name), pConstraint(nullptr) {
    }

    virtual void applyConstraint(bool bIgnoreSameState = true, Constraint *pConstraint = nullptr) {
      if ( !pConstraint ) {
        pConstraint = this->pConstraint;
      }
      if (pConstraint) {
        if ( automation::bSynchronizing && !pConstraint->isSynchronizable() ) {
          return;
        }
        bool bTestResult = pConstraint->test();
        if ( pPrerequisiteConstraint && !pPrerequisiteConstraint->test() ) {
          if ( bConstraintPassed ) {
            bConstraintPassed = false;
            constraintResultChanged(false);
          }
        } else if (!bIgnoreSameState || bTestResult != bConstraintPassed) {
          bConstraintPassed = bTestResult;
          constraintResultChanged(bTestResult);
        }
      }
    }

    virtual void constraintResultChanged(bool bConstraintResult) = 0;

    virtual bool testConstraint() {
      return pConstraint ? pConstraint->test() : true;
    }

    virtual Constraint* getConstraint() {
      return pConstraint;
    }

    virtual void setConstraint(Constraint* pConstraint) {
      this->pConstraint = pConstraint;
    }
    
    virtual void setup() = 0;

  protected:
    Constraint *pConstraint = nullptr;
    Constraint *pPrerequisiteConstraint = nullptr;
    vector<Capability *> capabilities;
    bool bInitialized = false;
    bool bConstraintPassed = false;
  };


}


#endif //AUTOMATION_DEVICE_H
