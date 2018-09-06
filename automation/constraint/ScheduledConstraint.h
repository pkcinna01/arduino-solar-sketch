#ifndef AUTOMATION_TIME_CONSTRAINT_H
#define AUTOMATION_TIME_CONSTRAINT_H

#include "Constraint.h"
#include "BooleanConstraint.h"
#include "NestedConstraint.h"
#include <ctime>
#include <vector>

using namespace std;

namespace automation {

  // cron like scheduling
  class ScheduledConstraint : public NestedConstraint {

  public:

    struct RangeVector : public vector<pair<uint16_t,uint16_t>> {
      int *pMember;
      RangeVector(int* pMember) : pMember(pMember) {
      }

      RangeVector& operator=(const vector<pair<uint16_t,uint16_t>>& rhs) {
        vector<pair<uint16_t,uint16_t>>::operator=(rhs);
        return *this;
      }

      bool check() {
        int val = *pMember;
        for ( auto range : *this ) {
          if ( val >= range.first && val <= range.second )
            return true;
        }
        return empty();
      }
    };

    struct tm checkTime;

    RangeVector seconds = { &checkTime.tm_sec };
    RangeVector minutes = { &checkTime.tm_min };
    RangeVector hours = { &checkTime.tm_hour };
    RangeVector weekDays = { &checkTime.tm_wday };
    RangeVector monthDays = { &checkTime.tm_mday };
    RangeVector months = { &checkTime.tm_mon };
    RangeVector years = { &checkTime.tm_year };
    vector<RangeVector*> rangeVectorList = { &years, &months, &monthDays, &weekDays, &hours, &minutes, &seconds };

    ScheduledConstraint(Constraint *pConstraint = &automation::PASS_CONSTRAINT) :
        NestedConstraint(pConstraint,"SCHEDULED") {
    }

    bool outerCheckValue(bool bInnerCheckResult) override {
      return bInnerCheckResult ? checkRanges() : false;
    }

    bool checkRanges() {
      time_t now = std::time(nullptr);
      checkTime = *localtime(&now);
      for( auto unitRanges : rangeVectorList) {
        if ( !unitRanges->check() ) {
          return false;
        }
      }
      return true;
    }
  };
}
#endif
