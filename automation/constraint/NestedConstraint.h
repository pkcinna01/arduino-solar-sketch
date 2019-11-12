#ifndef SOLAR_IFTTT_NESTEDCONSTRAINT_H
#define SOLAR_IFTTT_NESTEDCONSTRAINT_H

#include "Constraint.h"

namespace automation {

  class NestedConstraint : public Constraint {
  public:
    explicit NestedConstraint(Constraint *pConstraint) {
      children.push_back(pConstraint);
    }

    virtual bool outerCheckValue(bool bInnerResult) = 0;

    bool checkValue() override {
      return outerCheckValue(inner()->test()); // need to honor delays of inner constraint so cannot call checkValue directly
      //return outerCheckValue(pConstraint->checkValue());
    }

    Constraint* inner() const {
      return children[0];
    }

    string getTitle() const override {
      string title = getType();
      title += "(";
      title += inner()->getTitle();
      title += ")";
      return title;
    }

  };

}
#endif //SOLAR_IFTTT_NESTEDCONSTRAINT_H
