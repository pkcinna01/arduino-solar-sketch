#ifndef SOLAR_IFTTT_BOOLEANCONSTRAINT_H
#define SOLAR_IFTTT_BOOLEANCONSTRAINT_H

#include "Constraint.h"

namespace automation {

  class BooleanConstraint : public Constraint {

    const bool bResult;

  public:
    RTTI_GET_TYPE_IMPL(automation,Boolean)

    explicit BooleanConstraint(bool bResult) : bResult(bResult) {
      bPassed = bResult;
    }

    bool checkValue() override {
      return bResult;
    }

    string getTitle() const override {
      return bResult ? "PASS" : "FAIL";
    }
  };

  static BooleanConstraint FAIL_CONSTRAINT(false);
  static BooleanConstraint PASS_CONSTRAINT(true);

}
#endif //SOLAR_IFTTT_BOOLEANCONSTRAINT_H
