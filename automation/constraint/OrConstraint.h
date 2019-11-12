#ifndef AUTOMATION_ORCONSTRAINT_H
#define AUTOMATION_ORCONSTRAINT_H

#include "CompositeConstraint.h"

#include <algorithm>

namespace automation {

  class OrConstraint : public CompositeConstraint {
  public:
    RTTI_GET_TYPE_IMPL(automation,Or)
    
    OrConstraint(const vector<Constraint *> &constraints) :
        CompositeConstraint("OR", constraints) {
    }

    bool checkValue() override {
      bool bResult = children.empty();
      for (Constraint *pConstraint : children) {
        if (pConstraint->test()) {
          bResult = true;
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
