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
    }

    RTTI_GET_TYPE_DECL;
    
    virtual void applyConstraint(bool bIgnoreSameState = true, Constraint *pConstraint = nullptr);

    virtual void constraintResultChanged(bool bConstraintResult) = 0;

    virtual void print(json::JsonStreamWriter& w, bool bVerbose=false, bool bIncludePrefix=true) const override;

    virtual void printVerboseExtra(json::JsonStreamWriter& w) const { w.noPrefixPrintln(""); };

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


  class Devices : public NamedItemVector<Device*> {
  public:
    Devices(){}
    Devices( vector<Device*>& devices ) : NamedItemVector<Device*>(devices) {}
    Devices( vector<Device*> devices ) : NamedItemVector<Device*>(devices) {}
  };


}


#endif //AUTOMATION_DEVICE_H
