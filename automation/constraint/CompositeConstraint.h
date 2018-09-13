#ifndef AUTOMATION_COMPOSITECONSTRAINT_H
#define AUTOMATION_COMPOSITECONSTRAINT_H

#include "Constraint.h"
#include "../Automation.h"

#include <vector>
using namespace std;

namespace automation {

  class CompositeConstraint : public Constraint {
  public:
    vector<Constraint *> constraints;
    string strJoinName;
    bool bShortCircuit = false;

    CompositeConstraint(const string &strJoinName, const vector<Constraint *> &constraints) :
        strJoinName(strJoinName),
        constraints(constraints) {
    }

    string getTitle() override {
      string title = "(";
      for (int i = 0; i < constraints.size(); i++) {
        title += constraints[i]->getTitle();
        if (i + 1 < constraints.size()) {
          title += " ";
          title += strJoinName;
          title += " ";
        }
      }
      title += ")";
      return title;
    }
  };

}

#endif
