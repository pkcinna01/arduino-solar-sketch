
#include "Constraint.h"
#include "ConstraintEventHandler.h"

#include "../json/JsonStreamWriter.h"


namespace automation {

  bool Constraint::test()
  {
    if ( !bEnabled ) {
      return bPassed;
    }
    Mode resolvedMode = mode;
    if ( mode != TEST_MODE ) {
      if (mode&REMOTE_MODE) {
        if ( pRemoteExpiredOp->test() ) {
          // process locally because remote setting has expired
          resolvedMode = mode-REMOTE_MODE;
          if ( resolvedMode == 0 ) {
            // just return old result if REMOTE with no qualifiers
            checkValue(); // composite and nested constraints need checkValue call for deferred state tracking
            return bPassed;
          }
        } else {            
          // honor value set remotely but call checkValue to update transition and delay states
          checkValue(); // composite and nested constraints need checkValue call for deferred state tracking
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
      unsigned long durationMs = automation::millisecs()-changeTimeMs;
      ConstraintEventHandlerList::instance.resultChanged(this,bPassed,durationMs);
      listeners.resultChanged(this,bPassed,durationMs);
      changeTimeMs = automation::millisecs();
    } else if ( deferredResultCnt ) {
      deferredResultCnt = 0;
      unsigned long lastDeferredTimeMs = deferredTimeMs;
      deferredTimeMs = millisecs();
      unsigned long durationMs = deferredTimeMs-lastDeferredTimeMs;
      ConstraintEventHandlerList::instance.deferralCancelled(this,bPassed,durationMs);
      listeners.deferralCancelled(this,bPassed,durationMs);
    } else {
      unsigned long durationMs = automation::millisecs()-deferredTimeMs;
      ConstraintEventHandlerList::instance.resultSame(this,bPassed,durationMs);
      listeners.resultSame(this,bPassed,durationMs);
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
          if ( mode & (FAIL_MODE|PASS_MODE) ) {
            overrideTestResult(mode&PASS_MODE); // do not wait for transition delays
          } else {
            test();
          }          
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
      } else if ( !strcasecmp_P(pszKey,PSTR("ENABLED")) ) {
        bEnabled = text::parseBool(pszVal);
        strResultValue = text::boolAsString(bEnabled);
        rtn = SetCode::OK;
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
      } else if ( !strcasecmp_P(pszKey,PSTR("remoteValueExpOp")) ) {
        if ( !strcasecmp_P(pszVal,PSTR("auto")) ) {
          setRemoteExpiredOp(&defaultRemoteExpiredOp);
          strResultValue = "auto (client watchdog)";
          rtn = SetCode::OK;
        } else if (!strncasecmp_P(pszKey,PSTR("delay:"),6) ) {
          float delayMs = atof(&pszVal[6]);
          RemoteExpiredDelayOp* pExpOp = new RemoteExpiredDelayOp(delayMs);
          setRemoteExpiredOp(pExpOp);
          strResultValue = text::asString(pExpOp->delayMs);
          strResultValue += " (millisecs)";
          rtn = SetCode::OK;
        } else {
          if (pRespStream) {
            if (pRespStream->rdbuf()->in_avail()) {
              (*pRespStream) << ", ";
            }
            (*pRespStream) << RVSTR("Not a valid remote value expriation op (auto|delay): ") << pszVal;
          }
          rtn = SetCode::Error;
        }
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
    w.printlnBoolObj(F("enabled"), bEnabled, ",");
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
      w.printKey(F("remoteValueExpOp"));
      pRemoteExpiredOp->print(w);
      w.noPrefixPrintln(",");
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



