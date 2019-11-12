#include "Device.h"
#include "../capability/Capability.h"
#include "../json/JsonStreamWriter.h"


namespace automation {

void Device::applyConstraint(bool bIgnoreSameState, Constraint *pConstraint) {
  
  if ( !pConstraint ) {
    pConstraint = this->pConstraint;
  }
  if (pConstraint) {
    bool bLastPassed = pConstraint->isPassed();
    bool bPassed = pConstraint->test();
    //if (!bIgnoreSameState || bPassed != bLastPassed ) {
    //  constraintResultChanged(bPassed);
    //}
  }
}

#define CAPABILITY_PREFIX_SIZE 11
#define CONSTRAINT_PREFIX_SIZE 11
SetCode Device::setAttribute(const char* pszKey, const char* pszVal, ostream* pRespStream) {
  SetCode rtn = NamedContainer::setAttribute(pszKey,pszVal,pRespStream);
  if ( rtn == SetCode::Ignored ) {
    if ( pConstraint && !strncasecmp_P(pszKey,PSTR("CONSTRAINT."),CONSTRAINT_PREFIX_SIZE) ) {
      const char* pszConstraintKey = &pszKey[CONSTRAINT_PREFIX_SIZE];
      rtn = pConstraint->setAttribute(pszConstraintKey,pszVal,pRespStream);
      applyConstraint();
    } else if ( !strncasecmp_P(pszKey,PSTR("CAPABILITY."),CAPABILITY_PREFIX_SIZE) ) {
      const char* pszTypePattern = &pszKey[CAPABILITY_PREFIX_SIZE];
      for (auto cap : capabilities) {
        if (text::WildcardMatcher::test(pszTypePattern,cap->getType().c_str())) {
          SetCode code = cap->setAttribute(RVSTR("value"),pszVal,pRespStream);
          if ( code != SetCode::Ignored && rtn != SetCode::Error ) {
            rtn = code;
          }
        }
      }
    }
  }
  return rtn;
}

void Device::print(json::JsonStreamWriter& w, bool bVerbose, bool bIncludePrefix) const {
  if ( bIncludePrefix ) w.println("{"); else w.noPrefixPrintln("{");
  w.increaseDepth();
  w.printlnStringObj(F("name"),name,",");
  w.printlnNumberObj(F("id"), (unsigned long) id, ",");
  if ( bVerbose ) {
    if ( pConstraint ) {
    w.printKey(F("constraint"));
      pConstraint->print(w,bVerbose,json::PrefixOff);
      w.noPrefixPrintln(",");
    }
    w.printlnVectorObj(F("capabilities"), capabilities,",", bVerbose);
    printVerboseExtra(w);
  }    
  w.printlnStringObj(F("type"),getType());    
  w.decreaseDepth();
  w.print("}");
}



}
