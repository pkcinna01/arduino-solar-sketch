
#ifndef _AUTOMATION_ATTRIBUTE_CONTAINER_H_
#define _AUTOMATION_ATTRIBUTE_CONTAINER_H_

#include "json/Printable.h"

#include <string>
#include <functional>
#include <utility>

namespace automation {

  typedef unsigned char NumericIdentifierValue;
  const unsigned long NumericIdentifierMax = (1ul<<sizeof(NumericIdentifierValue)*8)-1;

  struct NumericIdentifier {
    
    NumericIdentifierValue id;

    template <typename T>
    static NumericIdentifierValue assignId(T* t) {
      static NumericIdentifierValue idGenerator = 0;
      t->id = ++idGenerator;
      return t->id;
    }

  };

  enum class SetCode { OK, Error, Ignored };

  class AttributeContainer : public NumericIdentifier, public json::Printable {

    public:

    virtual SetCode setAttribute(const char* pszKey, const char* pszVal, ostream* pResponseStream = nullptr) {
      return SetCode::Ignored;
    }
    
    virtual std::string getTitle() const = 0;

    virtual void printVerboseExtra(json::JsonStreamWriter& w) const {}

  };


  class NamedContainer : public AttributeContainer {

    public:

    mutable std::string name;
    
    NamedContainer(const std::string& name) : name(name) {
    }

    SetCode setAttribute(const char* pszKey, const char* pszVal, ostream* pResponseStream = nullptr) override {
      SetCode rtn = SetCode::Ignored;
      if ( pszKey && !strcasecmp_P(pszKey,PSTR("NAME")) ) {
        name = pszVal;
        rtn = SetCode::OK;
        if ( pResponseStream ) {
          (*pResponseStream) << "'" << getTitle() << "' " << pszKey << "=" << pszVal;
        }
      }
      return rtn;
    }

    virtual std::string getTitle() const override {
      return name;
    }
  };

  template<typename ContainerT>
  class AttributeContainerVector : public std::vector<ContainerT> {
  public:

    AttributeContainerVector() : std::vector<ContainerT>(){}
    AttributeContainerVector( std::vector<ContainerT>& v ) : std::vector<ContainerT>(v) {}
    
    template<typename IteratorT>
    AttributeContainerVector( const IteratorT& beginIt,  const IteratorT& endIt ) : std::vector<ContainerT>(beginIt,endIt) {}

    template<typename ResultContainerT>
    std::vector<ResultContainerT>& findByTitleLike( const char* pszWildCardPattern, std::vector<ResultContainerT>& resultVec, bool bInclude = true) {
      if ( pszWildCardPattern == nullptr || strlen(pszWildCardPattern) == 0 ) {
        return resultVec;
      }
      for( auto item : *this ) {
        if (bInclude==text::WildcardMatcher::test(pszWildCardPattern,item->getTitle().c_str()) ) {
          resultVec.push_back( (ResultContainerT) item);
        }
      }
      return resultVec;
    };    

    template<typename ResultContainerT>
    std::vector<ResultContainerT>& findById( unsigned long id, std::vector<ResultContainerT>& resultVec) {
      if ( id <= NumericIdentifierMax ) {
        for( auto item : *this ) {
          if ( item->id == id ) {
            resultVec.push_back((ResultContainerT)item);
            break;
          }
        }
      }
      return resultVec;
    }

    template<typename ResultContainerT>
    std::vector<ResultContainerT>& findByIds( std::vector<unsigned long> ids, std::vector<ResultContainerT>& resultVec) {
      for( auto id : ids ) {
        findById(id,resultVec);
      }
      return resultVec;
    }

    AttributeContainerVector findById( unsigned long id ) {      
      AttributeContainerVector resultVec;
      findById( id, resultVec );
      return resultVec;
    }
  };

}

#endif
