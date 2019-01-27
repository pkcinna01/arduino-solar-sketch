#ifndef AUTOMATION_CONSTRAINT_H
#define AUTOMATION_CONSTRAINT_H

#include "../Automation.h"
#include "../json/JsonStreamWriter.h"
#include "../AttributeContainer.h"

#include <string>
#include <iostream>
#include <string.h>

using namespace std;

namespace automation {

  class Constraint : public AttributeContainer {

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
    virtual bool test();

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

    unsigned long getPassDelayMs() const { return passDelayMs; }
    unsigned long getFailDelayMs() const { return failDelayMs; }
    unsigned long getPassMargin() const { return passMargin; }
    unsigned long getFailMargin() const { return failMargin; }
    unsigned long getDeferredRemainingMs() const { return max(0UL,(bPassed?failDelayMs:passDelayMs) - deferredDuration()); }
    bool isDeferred() const { return deferredResultCnt > 0; }

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

    void print(json::JsonStreamWriter& w, bool bVerbose=false, bool bIncludePrefix=true) const override;

    protected:

    bool bPassed = false;
    unsigned long deferredTimeMs = 0, changeTimeMs { automation::millisecs() };
    unsigned int deferredResultCnt = 0;
    float passMargin = 0;
    float failMargin = 0;
    virtual void setPassed(bool bPassed);
    
    unsigned long deferredDuration() const {
        unsigned long nowMs = millisecs();
        return (nowMs - deferredTimeMs);
    }

  };
}
#endif
