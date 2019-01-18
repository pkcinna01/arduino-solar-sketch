#ifndef AUTOMATION_ANDCONSTRAINT_H
#define AUTOMATION_ANDCONSTRAINT_H

#include "CompositeConstraint.h"

namespace automation {

  class AndConstraint : public CompositeConstraint {
  public:
    RTTI_GET_TYPE_IMPL(automation,And)
    
    AndConstraint(const vector<Constraint *> &constraints) :
        CompositeConstraint("AND", constraints) {
    }

    bool checkValue() override {
      bool bResult = true;
      for (Constraint *pConstraint : constraints) {
        if ( automation::bSynchronizing && !pConstraint->isSynchronizable() ) {
          continue;
        }
        if (!pConstraint->test()) {
          bResult = false;
          if ( bShortCircuit ) {
            break;
          }
        }
      }
      return bResult;
    }
  };
}

#endif
