#ifndef AUTOMATION_ORCONSTRAINT_H
#define AUTOMATION_ORCONSTRAINT_H

#include "CompositeConstraint.h"

#include <algorithm>

namespace automation {

  class OrConstraint : public CompositeConstraint {
  public:
    OrConstraint(const vector<Constraint *> &constraints) :
        CompositeConstraint("OR", constraints) {
    }

    bool checkValue() override {
      auto filter = [](Constraint* pConstraint)->bool{ return !(automation::bSynchronizing && !pConstraint->isSynchronizable()); };
      vector<Constraint*> filteredConstraints;
      std::copy_if(constraints.begin(), constraints.end(), std::back_inserter(filteredConstraints), filter);
      bool bResult = filteredConstraints.empty();
      for (Constraint *pConstraint : filteredConstraints) {
        if (pConstraint->test()) {
          bResult = true;
          if ( !bApplyAfterShortCircuit ) {
            break;
          }
        }
      }
      return bResult;
    }
  };

}

#endif
