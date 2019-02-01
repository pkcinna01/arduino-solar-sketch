#ifndef AUTOMATION_CAPABILITY_H
#define AUTOMATION_CAPABILITY_H

#include "../device/Device.h"
#include "../AttributeContainer.h"

#include <string>
#include <algorithm>
#include <functional>
#include <set>

using namespace std;

namespace automation {

  // Try to handle any value type but lean on double for now
  //
  class Capability : public AttributeContainer {

  public:

    double value = 0;

    static std::set<Capability*>& all(){
      static std::set<Capability*> all;
      return all;
    }    

    Capability(const Device* pDevice) : pDevice(pDevice) {
      assignId(this);
      all().insert(this);
    };

    virtual ~Capability() {
      all().erase(this);
    }
    virtual double getValueImpl() const = 0;
    virtual bool setValueImpl(double dVal) = 0;
    RTTI_GET_TYPE_DECL;

    virtual double getValue() const {
      return getValueImpl();
    }

    virtual bool setValue(double dVal) {
      bool bOk = setValueImpl(dVal);
      if ( bOk ) {
        notifyListeners();
      }
      return bOk;
    };

    bool asBoolean() const {
      return getValue() != 0;
    }

    const string getOwnerName() const {
        return pDevice ? pDevice->name : "";
    }

    bool setValue(bool bVal) {
      return setValue( (double) (bVal ? 1.0 : 0) );
    };

    virtual bool setValue(const string& strVal) {
      std::istringstream ss(strVal);
      double d;
      if (ss >> d ) {
        return setValue(d);
      } else {
        logBuffer << F("WARNING ") << __PRETTY_FUNCTION__ <<  F(" failed parsing ") << strVal << F(" to double.") << endl;
        return false;
      }
    }

    virtual SetCode setAttribute(const char* pszKey, const char* pszVal, ostream* pRespStream = nullptr) override;

    virtual void notifyListeners() {
      for( CapabilityListener* pListener : listeners) {
        pListener->valueSet(this,getValue());
      }
    };

    struct CapabilityListener {
      virtual void valueSet(const Capability* pCapability, double value) = 0;
    };

    vector<CapabilityListener*> listeners;

    virtual void addListener(CapabilityListener* pListener){
      listeners.push_back(pListener);
    }

    virtual void removeListener(CapabilityListener* pListener){
      listeners.erase(std::remove(listeners.begin(), listeners.end(), pListener), listeners.end());
    }

    virtual string getTitle() const {
      string str(getType());
      str += " '";
      str += getOwnerName();
      str += "'";
      return str;
    }

    virtual const string getDeviceName() const {
      return pDevice ? pDevice->name : "";
    }

    virtual void print(json::JsonStreamWriter& w, bool bVerbose=false, bool bIncludePrefix=true) const;

  protected:

    const Device* pDevice;

  };


 class Capabilities : public AttributeContainerVector<Capability*> {
  public:
    Capabilities(){}
    Capabilities( vector<Capability*>& c ) : AttributeContainerVector<Capability*>(c) {}
    Capabilities( vector<Capability*> c ) : AttributeContainerVector<Capability*>(c) {}
    Capabilities( set<Capability*>& c ) : AttributeContainerVector<Capability*>(c.begin(),c.end()) {}
  };
}

#endif //AUTOMATION_CAPABILITY_H
