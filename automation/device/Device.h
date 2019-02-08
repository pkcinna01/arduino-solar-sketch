#ifndef AUTOMATION_DEVICE_H
#define AUTOMATION_DEVICE_H

#include "../json/JsonStreamWriter.h"
#include "../constraint/Constraint.h"
#include "../AttributeContainer.h"

#include <vector>
#include <string>
#include <memory>

using namespace std;

namespace automation {

  class Capability;

  class Device : public NamedContainer {
  public:

    Constraint *pConstraint = nullptr;
    vector<Capability *> capabilities;
    bool bError;

    Device(const string &name) :
        NamedContainer(name), pConstraint(nullptr), bError(false) {
      assignId(this);
    }

    RTTI_GET_TYPE_DECL;
    
    virtual void applyConstraint(bool bIgnoreSameState = true, Constraint *pConstraint = nullptr);

    virtual void constraintResultChanged(bool bConstraintResult) = 0;

    virtual void print(json::JsonStreamWriter& w, bool bVerbose=false, bool bIncludePrefix=true) const override;

    virtual Constraint* getConstraint() {
      return pConstraint;
    }

    virtual void setConstraint(Constraint* pConstraint) {
      this->pConstraint = pConstraint;
    }
    
    virtual void setup() = 0;
    
    virtual SetCode setAttribute(const char* pszKey, const char* pszVal, ostream* pRespStream = nullptr) override;

  protected:
    bool bInitialized = false;

  };


  class Devices : public AttributeContainerVector<Device*> {
  public:
    Devices(){}
    Devices( vector<Device*>& devices ) : AttributeContainerVector<Device*>(devices) {}
    Devices( vector<Device*> devices ) : AttributeContainerVector<Device*>(devices) {}
  };

}


#endif //AUTOMATION_DEVICE_H
