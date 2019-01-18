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

  class Device : public AttributeContainer {
  public:

    Constraint *pConstraint = nullptr;
    vector<Capability *> capabilities;
    bool bError;

    Device(const string &name) :
        AttributeContainer(name), pConstraint(nullptr), bError(false) {
    }

    RTTI_GET_TYPE_DECL;
    
    virtual void applyConstraint(bool bIgnoreSameState = true, Constraint *pConstraint = nullptr);

    virtual void constraintResultChanged(bool bConstraintResult) = 0;

    virtual void print(int depth, bool bVerbose);

    virtual void print(int depth = 0) { 
      print(depth,false); 
    }
    
    virtual void printVerbose(int depth = 0 ) { 
      print(depth,true); 
    }

    virtual void printVerboseExtra(int depth = 1);

    virtual Constraint* getConstraint() {
      return pConstraint;
    }

    virtual void setConstraint(Constraint* pConstraint) {
      this->pConstraint = pConstraint;
    }
    
    virtual void setup() = 0;
    
    virtual bool setAttribute(const char* pszKey, const char* pszVal, ostream* pRespStream = nullptr) override;
    
    friend std::ostream &operator<<(std::ostream &os, const Device &d) {
      os << F("\"Device\": { \"name\": \"") << d.name << "\" }";
      return os;
    }

  protected:
    bool bInitialized = false;
  };


  class Devices : public AutomationVector<Device*> {
  public:
    Devices(){}
    Devices( vector<Device*>& devices ) : AutomationVector<Device*>(devices) {}
    Devices( vector<Device*> devices ) : AutomationVector<Device*>(devices) {}
  };


}


#endif //AUTOMATION_DEVICE_H
