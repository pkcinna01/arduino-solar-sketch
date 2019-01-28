#include "Device.h"
#include "../capability/Capability.h"
#include "../json/JsonStreamWriter.h"


namespace automation {

void Device::applyConstraint(bool bIgnoreSameState, Constraint *pConstraint) {
  if ( !pConstraint ) {
    pConstraint = this->pConstraint;
  }
  if (pConstraint) {
    if ( automation::bSynchronizing && !pConstraint->isSynchronizable() ) {
        return;
    }
    bool bLastPassed = pConstraint->isPassed();
    bool bPassed = pConstraint->test();
    if (!bIgnoreSameState || bPassed != bLastPassed ) {
      constraintResultChanged(bPassed);
    }
  }
}

#define CAPABILITY_PREFIX_SIZE 11
#define CONSTRAINT_PREFIX_SIZE 11
SetCode Device::setAttribute(const char* pszKey, const char* pszVal, ostream* pRespStream) {
  SetCode rtn = NamedContainer::setAttribute(pszKey,pszVal,pRespStream);
  if ( rtn == SetCode::Ignored ) {
    if ( pConstraint && !strncasecmp_P(pszKey,PSTR("CONSTRAINT/"),CONSTRAINT_PREFIX_SIZE) ) {
      const char* pszConstraintKey = &pszKey[CONSTRAINT_PREFIX_SIZE];
      rtn = pConstraint->setAttribute(pszConstraintKey,pszVal,pRespStream);
      applyConstraint();
    } else if ( !strncasecmp_P(pszKey,PSTR("CAPABILITY/"),CAPABILITY_PREFIX_SIZE) ) {
      float targetValue = !strcasecmp(pszVal, "ON") ? 1 : !strcasecmp(pszVal, "OFF") ? 0 : atof(pszVal);
      const char* pszTypePattern = &pszKey[CAPABILITY_PREFIX_SIZE];
      int matchCnt = 0;
      for (auto cap : capabilities) {
        if (text::WildcardMatcher::test(pszTypePattern,cap->getType().c_str())) {
          cap->setValue(targetValue);
          matchCnt++;
          if (pRespStream) {
            if (pRespStream->rdbuf()->in_avail()) {
              (*pRespStream) << ", ";
            }
            (*pRespStream) << cap->getTitle() << "=" << cap->getValue();
          }
        }
      }
      if ( matchCnt > 0 ) {
        rtn = SetCode::OK;
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
  w.printlnStringObj(F("type"),getType(),",");    
  w.printKey(F("constraint"));
  if ( bVerbose ) {
    pConstraint->print(w,bVerbose,json::PrefixOff);
    w.noPrefixPrintln(",");
    w.printVectorObj(F("capabilities"), capabilities);
    printVerboseExtra(w);
  } else {
    w.noPrefixPrint("{ \"") + F("state\": ") + (pConstraint->isPassed() ? "PASSED" : "FAILED");
    w.noPrefixPrintln(" }");
  }    
  w.decreaseDepth();
  w.print("}");
}



}
