#ifndef SOLAR_IFTTT_NOTCONSTRAINT_H
#define SOLAR_IFTTT_NOTCONSTRAINT_H

#include "Constraint.h"
#include "NestedConstraint.h"

namespace automation {

  class NotConstraint : public NestedConstraint {
  public:
    RTTI_GET_TYPE_IMPL(automation,Not)

     explicit NotConstraint(Constraint *pConstraint) : NestedConstraint(pConstraint) {
    }

    bool outerCheckValue(bool bInnerResult) override {
      return !bInnerResult;
    }
  };

}
#endif //SOLAR_IFTTT_BOOLEANCONSTRAINT_H
