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

    typedef unsigned char Mode;
    static const Mode OFF=0x0, ON=0x1, AUTO=0x2;

    static Mode parseMode(const char* pszMode)  {
        if (!strcasecmp("OFF", pszMode))
          return Constraint::OFF;
      if (!strcasecmp("ON", pszMode))
        return Constraint::ON;
      if (!strcasecmp("AUTO", pszMode))
        return Constraint::AUTO;
    }

    static string modeToString(Mode mode) {
      if ( mode == OFF ) {
        return "OFF";
      } else if ( mode == ON ) {
        return "ON";
      }
      return "AUTO";
    }

    Mode mode = AUTO;
    unsigned long passDelayMs = 0, failDelayMs = 0;

    virtual bool checkValue() = 0;
    virtual string getTitle() { return "Constraint"; }
    virtual bool isSynchronizable() { return true; }

    virtual bool test()
    {
      if ( mode == ON ) {
        deferredTimeMs = 0;
        return (bTestResult=true);
      } else if ( mode == OFF ) {
        deferredTimeMs = 0;
        return (bTestResult=false);
      }

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

    virtual void resetDeferredTime() {
      deferredTimeMs = 0;
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
