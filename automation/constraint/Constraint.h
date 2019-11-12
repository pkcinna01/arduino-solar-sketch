#ifndef AUTOMATION_CONSTRAINT_H
#define AUTOMATION_CONSTRAINT_H

#include "../Automation.h"
#include "../json/JsonStreamWriter.h"
#include "../AttributeContainer.h"
#include "ConstraintEventHandler.h"

#include <string>
#include <iostream>
#include <string.h>
#include <set>

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

    // access constraints without having to traverse all devices and nested constraints
    static std::set<Constraint*>& all(){
      static std::set<Constraint*> all;
      return all;
    }    

    Mode mode = TEST_MODE;

    bool bEnabled = true;
    
    Constraint() {
      assignId(this);
      all().insert(this);
    }

    Constraint(const std::vector<Constraint*>& children) : 
        children(children){
      assignId(this);
      all().insert(this);
    }

    virtual ~Constraint() {
      all().erase(this);
      if ( pRemoteExpiredOp != &defaultRemoteExpiredOp ) {
        delete pRemoteExpiredOp;
      }
    }

    virtual bool checkValue() = 0;
    virtual string getTitle() const { return getType(); }
    virtual bool test();
    
    struct RemoteExpiredOp {
      
      virtual bool test() { 
        // use global expiration based on last time a remote command was processed
        return automation::client::watchdog::isKeepAliveExpired(); 
      }

      virtual void reset(){} // place to track when a remote event occured

      virtual void print(json::JsonStreamWriter& w) {
        w.noPrefixPrintln("{");
        w.increaseDepth();
        w.printlnStringObj(F("type"), F("auto"),",");
        w.printlnBoolObj(F("expired"), test());
        w.decreaseDepth();
        w.print("}");
      }
    };

    static RemoteExpiredOp defaultRemoteExpiredOp;

    struct RemoteExpiredDelayOp : public RemoteExpiredOp {
      unsigned long delayMs;
      unsigned long attributeSetTimeMs; // each constraints remote status will expire individualy after a delay

      RemoteExpiredDelayOp( unsigned long delayMs ) : delayMs(delayMs), attributeSetTimeMs(0) {}
      
      bool test() override {
        return automation::millisecs() - attributeSetTimeMs > delayMs;
      }

      void reset() override {
        attributeSetTimeMs = automation::millisecs();
      }

      virtual void print(json::JsonStreamWriter& w) override {
        w.noPrefixPrintln("{");
        w.increaseDepth();
        w.printlnStringObj(F("type"), F("delay"),",");
        w.printlnNumberObj(F("delayMs"), delayMs, ",");
        w.printlnStringObj(F("elapsedMs"), automation::millisecs() - attributeSetTimeMs, ",");
        w.printlnBoolObj(F("expired"), test());
        w.decreaseDepth();
        w.print("}");
      }
    };
    
    RemoteExpiredOp* pRemoteExpiredOp {&defaultRemoteExpiredOp};
    
    void setRemoteExpiredOp(RemoteExpiredOp* pOp) {
      if ( pRemoteExpiredOp && pRemoteExpiredOp != &defaultRemoteExpiredOp ) {
        delete pRemoteExpiredOp;  // workaround for arduinostl not having unique_ptr
      }
      pRemoteExpiredOp = pOp;
    }

    SetCode setAttribute(const char* pszKey, const char* pszVal, ostream* pRespStream = nullptr) override;

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
    float getPassMargin() const { return passMargin; }
    float getFailMargin() const { return failMargin; }
    unsigned long getDeferredRemainingMs() const { return max(0UL,(bPassed?failDelayMs:passDelayMs) - deferredDuration()); }
    bool isDeferred() const { return deferredResultCnt > 0; }

    Constraint* findChildById(unsigned int id) const {
      for ( auto pChild : children ) {
        if ( pChild->id == id ) {
          return pChild;
        }
      }
      return nullptr;
    }

    void resetDeferredDuration() {
      deferredTimeMs = 0;
      deferredResultCnt = 0;
      for ( auto pChild : children ) {
        pChild->resetDeferredDuration();
      }
    }

    bool overrideTestResult(bool bNewResult) {
      deferredTimeMs = automation::millisecs();
      deferredResultCnt = 0;
      if ( bNewResult != isPassed() ) {
        setPassed(bNewResult);
      }
      return bPassed;
    }
    
    bool isPassed() const { return bPassed; };

    void print(json::JsonStreamWriter& w, bool bVerbose=false, bool bIncludePrefix=true) const override;

    bool isRemoteCompatible() {
      return (mode&REMOTE_MODE) > 0;
    }

    ConstraintEventHandlerList listeners;

    protected:

    vector<Constraint *> children;
    bool bPassed = false;
    unsigned long deferredTimeMs = 0, changeTimeMs { automation::millisecs() };
    unsigned int deferredResultCnt = 0;
    float passMargin = 0;
    float failMargin = 0;
    void setPassed(bool bPassed);
    
    unsigned long deferredDuration() const {
        unsigned long nowMs = millisecs();
        return (nowMs - deferredTimeMs);
    }

  };

  class Constraints : public AttributeContainerVector<Constraint*> {

  public:
    
    static DurationTimer& pauseEndTimer() {
      static DurationTimer pauseEndTimer(0);
      return pauseEndTimer;
    };

    static bool isPaused() {
      DurationTimer& timer = pauseEndTimer();
      return timer.getMaxDurationMs() != 0 && !timer.isExpired();
    }

    Constraints(){}
    Constraints( vector<Constraint*>& constraints ) : AttributeContainerVector<Constraint*>(constraints) {}
    Constraints( vector<Constraint*> constraints ) : AttributeContainerVector<Constraint*>(constraints) {}
    Constraints( set<Constraint*>& constraints ) : AttributeContainerVector<Constraint*>(constraints.begin(),constraints.end()) {}
  };
}
#endif
