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
      vector<Constraint*> filteredConstraints;
      //copy_if not supported in ArduinoSTL
      //auto filter = [](Constraint* pConstraint)->bool{ return !(automation::bSynchronizing && !pConstraint->isSynchronizable()); };
      //std::copy_if(constraints.begin(), constraints.end(), std::back_inserter(filteredConstraints), filter);
      for(Constraint* c: constraints){
        if ( !(automation::bSynchronizing && !c->isSynchronizable()) )
          filteredConstraints.push_back(c);
      }
      bool bResult = filteredConstraints.empty();
      for (Constraint *pConstraint : filteredConstraints) {
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
