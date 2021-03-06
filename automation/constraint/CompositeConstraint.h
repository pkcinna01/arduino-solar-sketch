#ifndef AUTOMATION_COMPOSITECONSTRAINT_H
#define AUTOMATION_COMPOSITECONSTRAINT_H

#include "Constraint.h"
#include "../Automation.h"

#include <vector>
using namespace std;

namespace automation {

  class CompositeConstraint : public Constraint {
  public:
    string strJoinName;
    bool bShortCircuit = false;

    CompositeConstraint(const string &strJoinName, const vector<Constraint *> &constraints) : 
        Constraint(constraints),
        strJoinName(strJoinName) {
    }

    string getTitle() const override {
      string title = "(";
      for (size_t i = 0; i < children.size(); i++) {
        title += children[i]->getTitle();
        if (i + 1 < children.size()) {
          title += " ";
          title += strJoinName;
          title += " ";
        }
      }
      title += ")";
      return title;
    }

    virtual void printVerboseExtra(json::JsonStreamWriter& w) const override {
      w.printlnBoolObj(F("shortCircuit"),bShortCircuit,",");
      w.printlnStringObj(F("joinName"),strJoinName,",");
    }

  };

}

#endif
