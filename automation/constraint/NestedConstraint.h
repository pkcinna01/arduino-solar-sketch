#ifndef SOLAR_IFTTT_NESTEDCONSTRAINT_H
#define SOLAR_IFTTT_NESTEDCONSTRAINT_H

#include "Constraint.h"

namespace automation {

  class NestedConstraint : public Constraint {
  public:
    explicit NestedConstraint(Constraint *pConstraint) :
        pConstraint(pConstraint) {
    }

    virtual bool outerCheckValue(bool bInnerResult) = 0;

    bool checkValue() override {
      return outerCheckValue(pConstraint->test()); // need to honor delays of inner constraint so cannot call checkValue directly
      //return outerCheckValue(pConstraint->checkValue());
    }

    string getTitle() const override {
      string title = getType();
      title += "(";
      title += pConstraint->getTitle();
      title += ")";
      return title;
    }

    bool isSynchronizable() const override {
      return pConstraint->isSynchronizable();
    }

  protected:
    Constraint *pConstraint;
  };

}
#endif //SOLAR_IFTTT_NESTEDCONSTRAINT_H
