
#include "Constraint.h"
#include "ConstraintEventHandler.h"

#include "../json/JsonStreamWriter.h"


namespace automation {

  bool Constraint::test()
  {
    Mode resolvedMode = mode;
    if ( mode != TEST_MODE ) {
      if (mode&REMOTE_MODE) {
        if ( pRemoteExpiredOp->test() ) {
          // remote connection lost so process locally
          resolvedMode = mode-REMOTE_MODE;
          if ( resolvedMode == INVALID_MODE ) {
            // leave old result on lost master connection
            return bPassed;
          }
        } else {
          return bPassed;
        }
      }
      if ( resolvedMode == PASS_MODE || resolvedMode == FAIL_MODE || resolvedMode == INVALID_MODE ) {
        return overrideTestResult( resolvedMode == PASS_MODE );
      }
    }

    bool bCheckPassed = checkValue();

    if ( !deferredTimeMs ) {
      // first test so ignore delays on changing state
      deferredTimeMs = millisecs();
      if ( deferredTimeMs == 0 )
        deferredTimeMs++;

      setPassed(bCheckPassed);
      
    } else if ( bPassed != bCheckPassed ) {

      if ( deferredResultCnt == 0 ) {
        deferredTimeMs = millisecs();
      }
      if ( bCheckPassed ) {
        if ( deferredDuration() >= passDelayMs ) {
          setPassed(true);
        } else {
          deferredResultCnt++;
        }
      } else {
        if ( deferredDuration() >= failDelayMs ) {
          setPassed(false);
        } else {
          deferredResultCnt++;
        }
      }
    } else { // test result has not changed
      if ( deferredResultCnt > 0 ) {
        // a delayed result change was in progress but the original test result occured
        setPassed(bCheckPassed);
      }
    }

    if ( deferredResultCnt == 1 ) {
      ConstraintEventHandlerList::instance.resultDeferred(this,bCheckPassed,bCheckPassed?passDelayMs:failDelayMs);
    }

    return bPassed;
  }


  void Constraint::setPassed(bool bPassed) {
    if ( bPassed != this->bPassed ) {
      deferredResultCnt = 0;
      this->bPassed = bPassed;
      ConstraintEventHandlerList::instance.resultChanged(this,bPassed,automation::millisecs()-changeTimeMs);
      changeTimeMs = automation::millisecs();
    } else if ( deferredResultCnt ) {
      deferredResultCnt = 0;
      unsigned long lastDeferredTimeMs = deferredTimeMs;
      deferredTimeMs = millisecs();
      ConstraintEventHandlerList::instance.deferralCancelled(this,bPassed,automation::millisecs()-lastDeferredTimeMs);
    } else {
      ConstraintEventHandlerList::instance.resultSame(this,bPassed,automation::millisecs()-deferredTimeMs);
    }
  }


  SetCode Constraint::setAttribute(const char* pszKey, const char* pszVal, ostream* pRespStream) {
    string strResultValue;
    SetCode rtn = AttributeContainer::setAttribute(pszKey,pszVal,pRespStream);
    if ( rtn == SetCode::Ignored ) {
      if ( !strcasecmp_P(pszKey,PSTR("MODE")) ) {
        Mode newMode = Constraint::parseMode(pszVal);
        if ( newMode != INVALID_MODE ) {
          mode = newMode;
          strResultValue = Constraint::modeToString(mode);
          test();
          rtn = SetCode::OK;
        } else {
          if (pRespStream) {
            if (pRespStream->rdbuf()->in_avail()) {
              (*pRespStream) << ", ";
            }
            (*pRespStream) << RVSTR("Not a valid mode: ") << pszVal;
          }
          rtn = SetCode::Error;
        }
      } else if ( !strcasecmp_P(pszKey,PSTR("PASSED")) ) {
        overrideTestResult(text::parseBool(pszVal));
        strResultValue = text::boolAsString(isPassed());
        rtn = SetCode::OK;
      } else if ( !strcasecmp_P(pszKey,PSTR("passDelayMs")) ) {
        setPassDelayMs(atol(pszVal));
        strResultValue = text::asString(getPassDelayMs());
        rtn = SetCode::OK;
      } else if ( !strcasecmp_P(pszKey,PSTR("failDelayMs")) ) {
        setFailDelayMs(atol(pszVal));
        strResultValue = text::asString(getFailDelayMs());
        rtn = SetCode::OK;
      } else if ( !strcasecmp_P(pszKey,PSTR("passMargin")) ) {
        setPassMargin(atof(pszVal));
        strResultValue = text::asString(getPassMargin());
        rtn = SetCode::OK;
      } else if ( !strcasecmp_P(pszKey,PSTR("failMargin")) ) {
        setFailMargin(atof(pszVal));
        strResultValue = text::asString(getFailMargin());
        rtn = SetCode::OK;
      } else if ( !strcasecmp_P(pszKey,PSTR("remoteExpiredDelayMinutes")) ) {
        if ( !strcasecmp_P(pszVal,PSTR("auto")) ) {
          setRemoteExpiredOp(&defaultRemoteExpiredOp);
          strResultValue = "auto (client watchdog)";
        } else {
          float delayMs = atof(pszVal)*MINUTES;
          setRemoteExpiredOp(new Constraint::RemoteExpiredDelayOp(delayMs));
          strResultValue = text::asString(delayMs);
          strResultValue += " (millisecs)";
        }
        rtn = SetCode::OK;
      }
      if (pRespStream && rtn == SetCode::OK ) {
        if (pRespStream->rdbuf()->in_avail()) {
          (*pRespStream) << ", ";
        }
        (*pRespStream) << "'" << getTitle() << "' " << pszKey << "=" << strResultValue;
      }
    }
    return rtn;
  }

  void Constraint::print(json::JsonStreamWriter& w, bool bVerbose, bool bIncludePrefix) const {
    if ( bIncludePrefix ) w.println("{"); else w.noPrefixPrintln("{");
    w.increaseDepth();
    w.printlnStringObj(F("title"), getTitle(), ",");
    w.printlnNumberObj(F("id"), (unsigned long) id, ",");
    w.printlnBoolObj(F("passed"), isPassed(), ",");
    if ( bVerbose ) {
      if ( !children.empty() ) {
        w.printlnVectorObj(F("children"), children, ",");
      }
      w.printlnNumberObj(F("passDelayMs"),getPassDelayMs(), ",");    
      w.printlnNumberObj(F("failDelayMs"),getFailDelayMs(), ",");    
      w.printlnNumberObj(F("passMargin"),getPassMargin(), ",");    
      w.printlnNumberObj(F("failMargin"),getFailMargin(), ",");    
      bool bIsDeferred = isDeferred();
      w.printlnBoolObj(F("isDeferred"),bIsDeferred, ",");    
      if ( bIsDeferred ) {
        w.printlnNumberObj(F("deferredRemainingMs"),getDeferredRemainingMs(), ",");    
      }
      w.printlnStringObj(F("mode"), Constraint::modeToString(mode).c_str(),",");  
      printVerboseExtra(w);  
    }
    w.printStringObj(F("type"), getType().c_str());
    w.decreaseDepth();
    w.noPrefixPrintln("");
    w.print("}");
  }

  Constraint::RemoteExpiredOp Constraint::defaultRemoteExpiredOp;
  ConstraintEventHandlerList ConstraintEventHandlerList::instance;

}



