#ifndef AUTOMATION_CONSTRAINT_H
#define AUTOMATION_CONSTRAINT_H

#include "../Automation.h"

#include <string>
#include <iostream>

using namespace std;

namespace automation {

  class Constraint {

    public:

    unsigned long passDelayMs = 0, failDelayMs = 0;

    virtual bool checkValue() = 0;
    virtual string getTitle() { return "Constraint"; }
    virtual bool isSynchronizable() { return true; }

    virtual bool test()
    {
      bool bCheckPassed = checkValue();

      if ( !deferredTimeMs ) {
        // first test so ignore delays on changing state
        deferredTimeMs = millisecs();
        if ( deferredTimeMs == 0 )
          deferredTimeMs++;

        setTestResult(bCheckPassed);
        
      } else if ( bTestResult != bCheckPassed ) {

        if ( deferredResultCnt == 0 ) {
              deferredTimeMs = millisecs();
        }
        if ( bCheckPassed ) {
          if ( deferredDuration() >= passDelayMs ) {
            setTestResult(true);
          } else {
            if ( !deferredResultCnt ) {
              logBuffer << "CONSTRAINT " << getTitle() << ": accept pending (delay:" << (passDelayMs-deferredDuration())/1000.0 << "s)..." << endl;
            }
            deferredResultCnt++;
          }
        } else {
          if ( deferredDuration() >= failDelayMs ) {
            setTestResult(false);
          } else {
            if ( !deferredResultCnt ) {
              logBuffer << "CONSTRAINT " << getTitle() << ": reject pending (delay:" << (failDelayMs-deferredDuration())/1000.0 << "s)..." << endl;
            }
            deferredResultCnt++;
          }
        }
      } else { // test result has not changed
        if ( deferredResultCnt > 0 ) {
          // a delayed result change was in progress but the original test 
          // result occured so reset change time and deferred cnt
          deferredTimeMs = millisecs();
          setTestResult(bCheckPassed);
        }
      } 

      return bTestResult;
    }

    virtual Constraint& setPassDelayMs(unsigned long delayMs) {
      passDelayMs = delayMs;
      return *this;
    }
    
    virtual Constraint& setFailDelayMs(unsigned long delayMs) {
      failDelayMs = delayMs;
      return *this;
    }

    virtual Constraint& setPassMargin(float passMargin) {
      this->passMargin = passMargin;
      return *this;
    }

    virtual Constraint& setFailMargin(float margin) {
      this->failMargin = margin;
      return *this;
    }

    protected:

    bool bTestResult = false;
    unsigned long deferredTimeMs = 0;
    unsigned int deferredResultCnt = 0;
    float passMargin = 0;
    float failMargin = 0;

    virtual void setTestResult(bool bTestResult) {

      logBuffer << "CONSTRAINT " << getTitle() << " set to " << bTestResult;
      if ( bTestResult != this->bTestResult || deferredResultCnt ) {
        if ( bTestResult != this->bTestResult ) {
          logBuffer << " (changed)";
        } else {
          logBuffer << " (reverting to old state after pending change)";
        }
        deferredResultCnt = 0;
        this->bTestResult = bTestResult;
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
