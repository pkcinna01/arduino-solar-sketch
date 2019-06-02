#ifndef AUTOMATION_CONSTRAINT_EVENT_HANDLER_H
#define AUTOMATION_CONSTRAINT_EVENT_HANDLER_H

//#include "Constraint.h"

namespace automation { 
    
  class Constraint;

  class ConstraintEventHandler {
    public:
    virtual void resultDeferred(Constraint* pConstraint,bool bNew,unsigned long delayMs) const {};
    virtual void resultChanged(Constraint* pConstraint,bool bNew,unsigned long lastDurationMs) const = 0;
    virtual void resultSame(Constraint* pConstraint,bool bVal,unsigned long lastDurationMs) const {};
    virtual void deferralCancelled(Constraint* pConstraint,bool bNew,unsigned long lastDurationMs) const {};
  };
  
  class ConstraintEventHandlerList : public ConstraintEventHandler, public std::vector<ConstraintEventHandler*> {
    public:    
    void resultDeferred(Constraint* pConstraint,bool bNew,unsigned long delayMs) const override {
      for( auto handler : *this ) handler->resultDeferred(pConstraint,bNew,delayMs);
    }
    void resultChanged(Constraint* pConstraint,bool bNew,unsigned long lastDurationMs) const override {
      for( auto handler : *this ) handler->resultChanged(pConstraint,bNew,lastDurationMs);
    }
    void resultSame(Constraint* pConstraint,bool bVal,unsigned long lastDurationMs) const override {
      for( auto handler : *this ) handler->resultSame(pConstraint,bVal,lastDurationMs);
    }
    void deferralCancelled(Constraint* pConstraint,bool bNew,unsigned long lastDurationMs) const override {
      for( auto handler : *this ) handler->deferralCancelled(pConstraint,bNew,lastDurationMs);
    }
    void add(ConstraintEventHandler* pHandler){
      push_back(pHandler);
    }
    void remove(ConstraintEventHandler* pHandler){
      this->erase(std::remove(begin(), end(), pHandler), end());
    }
    static ConstraintEventHandlerList instance;
  };

}
#endif
