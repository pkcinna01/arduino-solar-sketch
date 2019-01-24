
#ifndef _AUTOMATION_ATTRIBUTE_CONTAINER_H_
#define _AUTOMATION_ATTRIBUTE_CONTAINER_H_

#include "json/Printable.h"

#include <string>

namespace automation {

  class AttributeContainer : public json::Printable {

    public:

    virtual bool setAttribute(const char* pszKey, const char* pszVal, ostream* pResponseStream = nullptr){
      return true;
    }
    
    virtual std::string getTitle() const = 0;
  };


  class NamedAttributeContainer : public AttributeContainer {

    public:

    mutable std::string name;
    
    NamedAttributeContainer(const std::string& name) : name(name) {
    }

    bool setAttribute(const char* pszKey, const char* pszVal, ostream* pResponseStream = nullptr) override {

      if ( pszKey && !strcasecmp("name",pszKey) ) {
        name = pszVal;
      } else {
        return false;
      }
      if ( pResponseStream ) {
        (*pResponseStream) << "'" << getTitle() << "' " << pszKey << "=" << pszVal;
      }
      return true; // attribute found and set
    }

    virtual std::string getTitle() const override {
      return name;
    }
  };

}

#endif