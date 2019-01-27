
#include "Constraint.h"
#include "ConstraintEventHandler.h"

#include "../json/JsonStreamWriter.h"


namespace automation {

  bool Constraint::test()
  {
    Mode resolvedMode = mode;
    if ( mode != TEST_MODE ) {
      if (mode&REMOTE_MODE) {
        if ( automation::client::watchdog::isKeepAliveExpired() ) {
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
      if ( resolvedMode == PASS_MODE || resolvedMode == INVALID_MODE ) {
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


  void Constraint::print(json::JsonStreamWriter& w, bool bVerbose, bool bIncludePrefix) const {
    if ( bIncludePrefix ) w.println("{"); else w.noPrefixPrintln("{");
    w.increaseDepth();
    w.printlnStringObj(F("title"), getTitle().c_str() , ",");
    w.printlnStringObj(F("state"), isPassed() ? "PASSED" : "FAILED", ",");
    if ( bVerbose ) {
      w.printlnNumberObj(F("passDelayMs"),getPassDelayMs(), ",");    
      w.printlnNumberObj(F("failDelayMs"),getFailDelayMs(), ",");    
      bool bIsDeferred = isDeferred();
      w.printlnBoolObj(F("isDeferred"),bIsDeferred, ",");    
      if ( bIsDeferred ) {
        w.printlnNumberObj(F("deferredRemainingMs"),getDeferredRemainingMs(), ",");    
      }
      w.printlnStringObj(F("mode"), Constraint::modeToString(mode).c_str(),",");    
    }
    w.printStringObj(F("type"), getType().c_str());
    w.decreaseDepth();
    w.noPrefixPrintln("");
    w.print("}");
  }

  ConstraintEventHandlerList ConstraintEventHandlerList::instance;

}



