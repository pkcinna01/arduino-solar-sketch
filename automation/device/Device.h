#ifndef AUTOMATION_DEVICE_H
#define AUTOMATION_DEVICE_H

#include <vector>
#include <string>
#include <memory>
#include "../constraint/Constraint.h"
#include "../constraint/BooleanConstraint.h"

using namespace std;

namespace automation {

  class Capability;

  class Device {
  public:

    string name;
    Constraint *pConstraint = nullptr;
    vector<Capability *> capabilities;

    Device(const string &name) :
        name(name), pConstraint(&FAIL_CONSTRAINT) {
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

    virtual void print(int depth = 0);

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
    Constraint *pPrerequisiteConstraint = nullptr;
    bool bInitialized = false;
    bool bConstraintPassed = false;
  };

  class Devices : public vector<Device*> {
  public:
    Devices( vector<Device*>& devices )
      : vector<Device*>(devices) {
    }
    Devices( vector<Device*> devices )
        : vector<Device*>(devices) {
    }
  };


}


#endif //AUTOMATION_DEVICE_H
