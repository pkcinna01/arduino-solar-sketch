#ifndef AUTOMATION_CAPABILITY_H
#define AUTOMATION_CAPABILITY_H

#include "../device/Device.h"
#include "../AttributeContainer.h"

#include <string>
#include <algorithm>
#include <functional>

using namespace std;

namespace automation {

  // Try to handle any value type but lean on double for now
  //
  class Capability : public AttributeContainer {

  public:

    double value = 0;

    Capability(const Device* pDevice) : pDevice(pDevice) {
    };

    virtual double getValueImpl() const = 0;
    virtual void setValueImpl(double dVal) = 0;
    RTTI_GET_TYPE_DECL;

    virtual double getValue() const {
      return getValueImpl();
    }

    virtual void setValue(double dVal) {
      setValueImpl(dVal);
      notifyListeners();
    };

    virtual bool asBoolean() const {
      return getValue() != 0;
    }

    virtual const string getOwnerName() const {
        return pDevice ? pDevice->name : "";
    }

    virtual void setValue(bool bVal) {
      setValue( (double) (bVal ? 1.0 : 0) );
    };

    virtual void setValue(const string& strVal) {
      std::istringstream ss(strVal);
      double d;
      if (ss >> d ) {
        setValue(d);
      } else {
        logBuffer << F("WARNING ") << __PRETTY_FUNCTION__ <<  F(" failed parsing ") << strVal << F(" to double.") << endl;
      }
    }

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

    friend std::ostream &operator<<(std::ostream &os, const Capability &c) {
      os << c.getTitle() << " = " << text::asString(c.getValue());
      return os;
    }

  protected:

    const Device* pDevice;

  };

}

#endif //AUTOMATION_CAPABILITY_H
