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

  class Device : public NamedContainer, public ConstraintEventHandler {
  public:

    vector<Capability *> capabilities;
    mutable bool bError;

    Device(const string &name) :
        NamedContainer(name), bError(false) {
      assignId(this);
    }

    virtual ~Device() {
      if ( pConstraint ) {
        pConstraint->listeners.remove(this);
      }
    }

    RTTI_GET_TYPE_DECL;
    
    virtual void applyConstraint(bool bIgnoreSameState = true, Constraint *pConstraint = nullptr);

    virtual void print(json::JsonStreamWriter& w, bool bVerbose=false, bool bIncludePrefix=true) const override;

    virtual Constraint* getConstraint() const {
      return pConstraint;
    }

    virtual void setConstraint(Constraint* pConstraint) {
      if ( pConstraint ) {
        pConstraint->listeners.remove(this);
      }
      this->pConstraint = pConstraint;
      this->pConstraint->listeners.add(this);
    }

    bool isPassed() {
      if (!pConstraint) {
        return false;
      } else {
        return pConstraint->isPassed();
      }
    }    
    
    virtual void setup() = 0;
    
    virtual SetCode setAttribute(const char* pszKey, const char* pszVal, ostream* pRespStream = nullptr) override;

    Constraint* findConstraint(unsigned int id) const { 
      if ( !pConstraint ) {
        return nullptr;
      }
      if ( pConstraint->id == id ) {
        return pConstraint;
      } else {
        return pConstraint->findChildById(id);
      }
    }

  protected:
    bool bInitialized = false;

  private:
    Constraint *pConstraint = nullptr;

  };


  class Devices : public AttributeContainerVector<Device*> {
  public:
    Devices(){}
    Devices( vector<Device*>& devices ) : AttributeContainerVector<Device*>(devices) {}
    Devices( vector<Device*> devices ) : AttributeContainerVector<Device*>(devices) {}
    Device* findConstraintOwner(unsigned int id) const {
      for ( auto pDevice : *this ) {
        if ( pDevice->findConstraint(id) ) {
          return pDevice; //TODO - some constraints may be owned by multiple devices so should this return a list?
        }
      }
      return nullptr;
    }
  };

}


#endif //AUTOMATION_DEVICE_H
