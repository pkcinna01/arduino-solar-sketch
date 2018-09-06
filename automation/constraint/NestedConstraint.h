#ifndef SOLAR_IFTTT_NESTEDCONSTRAINT_H
#define SOLAR_IFTTT_NESTEDCONSTRAINT_H

#include "Constraint.h"

namespace automation {

  class NestedConstraint : public Constraint {
  public:
    explicit NestedConstraint(Constraint *pConstraint, const string &outerTitle = "NESTED") :
        pConstraint(pConstraint),
        outerTitle(outerTitle) {
    }

    virtual bool outerCheckValue(bool bInnerResult) = 0;

    bool checkValue() override {
      return outerCheckValue(pConstraint->checkValue());
    }

    string getTitle() override {
      string title = outerTitle;
      title += "(";
      title += pConstraint->getTitle();
      title += ")";
      return title;
    }

    bool isSynchronizable() override {
      return pConstraint->isSynchronizable();
    }

  protected:
    Constraint *pConstraint;
    string outerTitle;

  };

}
#endif //SOLAR_IFTTT_NESTEDCONSTRAINT_H
