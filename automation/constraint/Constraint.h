#ifndef AUTOMATION_CONSTRAINT_H
#define AUTOMATION_CONSTRAINT_H

#include "../Automation.h"

#include <string>
#include <iostream>
#include <string.h>

using namespace std;

namespace automation {

  class Constraint {

    public:
    RTTI_GET_TYPE_DECL;

    // REMOTE flag can be combined with PASS, FAIL, or TEST which allows a fall back if remote keep alive expired.
    // Examples:
    // 1) Pass constraint if keep alive from master expired: mode=REMOTE_MODE|PASS_MODE;
    // 2) Use local test if keep alive from master expired: mode=REMOTE_MODE|TEST_MODE;
    // 3) Skip local test and always pass constraint: mode=PASS_MODE;
    using Mode = unsigned char;
    const static Mode INVALID_MODE=0, FAIL_MODE = 0x1, PASS_MODE=0x2, TEST_MODE=0x4, REMOTE_MODE=0x8; 

    unsigned long passDelayMs = 0, failDelayMs = 0;

    static Mode parseMode(const char* pszMode)  {
      if (!strcasecmp_P(pszMode,PSTR("FAIL")))
          return FAIL_MODE;
      if (!strcasecmp_P(pszMode,PSTR("PASS")))
        return PASS_MODE;
      if (!strcasecmp_P(pszMode,PSTR("TEST")))
        return TEST_MODE;
      if (!strcasecmp_P(pszMode,PSTR("REMOTE")))
        return REMOTE_MODE;
      if (!strcasecmp_P(pszMode,PSTR("REMOTE_OR_FAIL")))
        return REMOTE_MODE|FAIL_MODE;
      if (!strcasecmp_P(pszMode,PSTR("REMOTE_OR_PASS")))
        return REMOTE_MODE|PASS_MODE;
      if (!strcasecmp_P(pszMode,PSTR("REMOTE_OR_TEST")))
        return REMOTE_MODE|TEST_MODE;
      return INVALID_MODE;
    }

    static string modeToString(Mode mode) {
      switch(mode) {
        case FAIL_MODE: return RVSTR("FAIL");
        case PASS_MODE: return RVSTR("PASS");
        case TEST_MODE: return RVSTR("TEST");
        case REMOTE_MODE: return RVSTR("REMOTE");
        case REMOTE_MODE|FAIL_MODE: return RVSTR("REMOTE_OR_FAIL");
        case REMOTE_MODE|PASS_MODE: return RVSTR("REMOTE_OR_PASS");
        case REMOTE_MODE|TEST_MODE: return RVSTR("REMOTE_OR_TEST");
        default: return RVSTR("INVALID");
      }
    }

    Mode mode = TEST_MODE;
    
    virtual bool checkValue() = 0;
    virtual string getTitle() const { return getType(); }
    virtual bool isSynchronizable() const { return true; }

    virtual bool test()
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
            if ( !deferredResultCnt ) {
              logBuffer << F("CONSTRAINT ") << getTitle() << F(": accept pending (delay:") << (passDelayMs-deferredDuration())/1000.0 << F("s)...") << endl;
            }
            deferredResultCnt++;
          }
        } else {
          if ( deferredDuration() >= failDelayMs ) {
            setPassed(false);
          } else {
            if ( !deferredResultCnt ) {
              logBuffer << F("CONSTRAINT ") << getTitle() << F(": reject pending (delay:") << (failDelayMs-deferredDuration())/1000.0 << F("s)...") << endl;
            }
            deferredResultCnt++;
          }
        }
      } else { // test result has not changed
        if ( deferredResultCnt > 0 ) {
          // a delayed result change was in progress but the original test 
          // result occured so reset change time and deferred cnt
          deferredTimeMs = millisecs();
          setPassed(bCheckPassed);
        }
      } 

      return bPassed;
    }

    Constraint& setPassDelayMs(unsigned long delayMs) {
      passDelayMs = delayMs;
      return *this;
    }
    
    Constraint& setFailDelayMs(unsigned long delayMs) {
      failDelayMs = delayMs;
      return *this;
    }

    Constraint& setPassMargin(float passMargin) {
      this->passMargin = passMargin;
      return *this;
    }

    Constraint& setFailMargin(float margin) {
      this->failMargin = margin;
      return *this;
    }

    unsigned long getPassDelayMs() { return passDelayMs; }
    unsigned long getFailDelayMs() { return failDelayMs; }
    unsigned long getPassMargin() { return passMargin; }
    unsigned long getFailMargin() { return failMargin; }

    virtual void resetDeferredTime() {
      deferredTimeMs = 0;
    }

    virtual bool overrideTestResult(bool bNewResult) {
      resetDeferredTime();
      if ( bNewResult != bPassed ) {
        setPassed(bNewResult);
      }
      return bPassed;
    }
    
    virtual bool isPassed() const { return bPassed; };
    virtual void print(int depth = 0) const;
    virtual void printVerbose(int depth = 0 ) const { print(depth); }

    friend std::ostream &operator<<(std::ostream &os, const Constraint &c) {
      os << F("\"Constraint\": { \"title\": \"") << c.getTitle() << F("\", \"passed\": ") << c.isPassed() << " }";
      return os;
    }

    protected:

    bool bPassed = false;
    unsigned long deferredTimeMs = 0;
    unsigned int deferredResultCnt = 0;
    float passMargin = 0;
    float failMargin = 0;

    virtual void setPassed(bool bPassed) {

      logBuffer << "CONSTRAINT " << getTitle() << " set to " << bPassed;
      if ( bPassed != this->bPassed || deferredResultCnt ) {
        if ( bPassed != this->bPassed ) {
          logBuffer << " (changed)";
        } else {
          logBuffer << " (reverting to old state after pending change)";
        }
        deferredResultCnt = 0;
        this->bPassed = bPassed;
      } else {
        logBuffer << " (no change)";
      }
      logBuffer << "." << endl;
    }

    unsigned long deferredDuration() {
        unsigned long nowMs = millisecs();
        return (nowMs - deferredTimeMs);
    }

  };
}
#endif
