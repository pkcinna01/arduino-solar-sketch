
#ifndef _AUTOMATION_ATTRIBUTE_CONTAINER_H_
#define _AUTOMATION_ATTRIBUTE_CONTAINER_H_

#include "json/Printable.h"

#include <string>
#include <functional>
#include <utility>

namespace automation {

  typedef unsigned char NumericIdentifierValue;
  const unsigned long NumericIdentifierMax = (2<<sizeof(NumericIdentifierValue))-1;

  struct NumericIdentifier {
    
    NumericIdentifierValue id;

    template <typename T>
    static void init( std::vector<T>& attrContainers) {
      unsigned char idGenerator = 0;
      for( const T& item : attrContainers ) {
        item->id = ++idGenerator;
      }
    }

  };

  class AttributeContainer : public NumericIdentifier, public json::Printable {

    public:

    virtual bool setAttribute(const char* pszKey, const char* pszVal, ostream* pResponseStream = nullptr){
      return true;
    }
    
    virtual std::string getTitle() const = 0;

  };


  class NamedContainer : public AttributeContainer {

    public:

    mutable std::string name;
    
    NamedContainer(const std::string& name) : name(name) {
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

  template<typename T>
  class NamedItemVector : public std::vector<T> {
  public:

    NamedItemVector() : std::vector<T>(){}
    NamedItemVector( std::vector<T>& v ) : std::vector<T>(v) {}

    std::vector<T>& findByNameLike( const char* pszWildCardPattern, std::vector<T>& resultVec, bool bInclude = true) {
      if ( pszWildCardPattern == nullptr || strlen(pszWildCardPattern) == 0 ) {
        return resultVec;
      }
      for( const T& item : *this ) {
        if (bInclude==text::WildcardMatcher::test(pszWildCardPattern,item->name.c_str()) ) {
          resultVec.push_back(item);
        }
      }
      return resultVec;
    };    

    using MatchFn = std::unary_function<T&,bool>;

    std::vector<T>& find( MatchFn matchFn, std::vector<T>& resultVec ) {
      for( const T& item : *this ) {
        if ( matchFn(item) ) {
          resultVec.push_back(item);
        }
      }
      return resultVec;
    }

    std::vector<T>& findById( NumericIdentifierValue id, std::vector<T>& resultVec) {
      for( const T& item : *this ) {
        if ( item->id == id ) {
          resultVec.push_back(item);
        }
      }
      return resultVec;
    }

    std::vector<T> findById( unsigned long id ) {      
      std::vector<T> resultVec;
      if ( id <= NumericIdentifierMax ) {
        findById( id, resultVec );
      }
      return resultVec;
    }
  };

  class NamedContainerVector : public NamedItemVector<NamedContainer*> {
  public:
    NamedContainerVector() : NamedItemVector<NamedContainer*>(){}
    NamedContainerVector( std::vector<NamedContainer*>& v ) : NamedItemVector<NamedContainer*>(v) {}
  };

}

#endif
