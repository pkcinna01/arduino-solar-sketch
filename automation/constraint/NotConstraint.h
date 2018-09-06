#ifndef SOLAR_IFTTT_NOTCONSTRAINT_H
#define SOLAR_IFTTT_NOTCONSTRAINT_H

#include "Constraint.h"
#include "NestedConstraint.h"

namespace automation {

  class NotConstraint : public NestedConstraint {
  public:
    explicit NotConstraint(Constraint *pConstraint) : NestedConstraint(pConstraint, "NOT") {
    }

    bool outerCheckValue(bool bInnerResult) override {
      return !bInnerResult;
    }
  };

}
#endif //SOLAR_IFTTT_BOOLEANCONSTRAINT_H
