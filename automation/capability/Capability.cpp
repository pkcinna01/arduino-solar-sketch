

#include "Capability.h"
#include "../json/JsonStreamWriter.h"

namespace automation {


SetCode Capability::setAttribute(const char* pszKey, const char* pszVal, ostream* pRespStream) {
  SetCode rtn = AttributeContainer::setAttribute(pszKey,pszVal,pRespStream);
  if ( rtn == SetCode::Ignored ) {
    if ( !strcasecmp_P(pszKey,PSTR("VALUE")) ) {
      float targetValue = !strcasecmp(pszVal, "ON") ? 1 : !strcasecmp(pszVal, "OFF") ? 0 : atof(pszVal);
      bool bOk = setValue(targetValue);
      if (pRespStream) {
        if (pRespStream->rdbuf()->in_avail()) {
          (*pRespStream) << ", ";
        }
        if ( bOk ) {
          (*pRespStream) << getTitle() << "=" << getValue();
        } else {
          (*pRespStream) << "'" << getTitle() << F("' new value rejected: ") << pszVal;
        }
      }
      rtn = bOk ? SetCode::OK : SetCode::Error;
    }
  }
  return rtn;
}

void Capability::print(json::JsonStreamWriter& w, bool bVerbose, bool bIncludePrefix) const
{
    if ( bIncludePrefix ) w.println("{"); else w.noPrefixPrintln("{");

    w.increaseDepth();
    w.printlnStringObj(F("type"),getType().c_str(),",");
    w.printlnStringObj(F("title"),getTitle().c_str(),",");
    w.printlnNumberObj(F("id"),(int) id,",");
    if ( bVerbose ) {
      printVerboseExtra(w);
      if ( pDevice ) {
        //pDevice->printlnObj(w,F("device"),",",false);
        w.printlnNumberObj(F("deviceId"), (int) pDevice->id,",");
      }
    }
    w.printlnNumberObj(F("value"), getValue() );
    w.decreaseDepth();
    w.print("}");
  }

}

