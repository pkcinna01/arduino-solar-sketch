#ifndef SOLAR_IFTTT_BOOLEANCONSTRAINT_H
#define SOLAR_IFTTT_BOOLEANCONSTRAINT_H

#include "Constraint.h"

namespace automation {

  class BooleanConstraint : public Constraint {

    const bool bResult;

  public:
    explicit BooleanConstraint(bool bResult) : bResult(bResult) {
    }

    bool checkValue() override {
      return bResult;
    }

    string getTitle() override {
      return bResult ? "PASS" : "FAIL";
    }
  };

  BooleanConstraint FAIL_CONSTRAINT(false);
  BooleanConstraint PASS_CONSTRAINT(true);

}
#endif //SOLAR_IFTTT_BOOLEANCONSTRAINT_H
