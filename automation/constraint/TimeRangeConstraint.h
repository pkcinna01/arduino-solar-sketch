#ifndef AUTOMATION_TIMERANGE_CONSTRAINT_H
#define AUTOMATION_TIMERANGE_CONSTRAINT_H

#include "Constraint.h"
#include "BooleanConstraint.h"
#include "NestedConstraint.h"
#include <ctime>
#include <vector>
#include <sstream>
#include <iomanip>

using namespace std;

namespace automation {

  class TimeRangeConstraint : public NestedConstraint {
  public:
    struct Time {
      int hour, minute, second;
    } beginTime, endTime;

    TimeRangeConstraint(Time beginTime, Time endTime, Constraint *pConstraint = &automation::PASS_CONSTRAINT) :
        NestedConstraint(pConstraint, "TIME_RANGE"),
        beginTime(beginTime),
        endTime(endTime) {
    }

    bool outerCheckValue(bool bInnerCheckResult) override {
      return bInnerCheckResult ? checkTimeRanges() : false;
    }

    bool checkTimeRanges() {
      time_t now = std::time(nullptr);
      struct tm checkTime = *localtime(&now);
      struct tm beginTm = checkTime, endTm = checkTime;
      beginTm.tm_hour = beginTime.hour;
      beginTm.tm_min = beginTime.minute;
      beginTm.tm_sec = beginTime.second;
      endTm.tm_hour = endTime.hour;
      endTm.tm_min = endTime.minute;
      endTm.tm_sec = endTime.second;
      now = mktime(&checkTime);
      time_t beginTimeT = mktime(&beginTm);
      time_t endTimeT = mktime(&endTm);
      return now >= beginTimeT && now <= endTimeT;
    }

    string getTitle() override {
      stringstream ss;
      ss << outerTitle << "[" << setfill('0') << setw(2) << beginTime.hour << ":"
         << setfill('0') << setw(2) << beginTime.minute << ":"
         << setfill('0') << setw(2) << beginTime.second
         << "-"
         << setfill('0') << setw(2) << endTime.hour << ":"
         << setfill('0') << setw(2) << endTime.minute << ":"
         << setfill('0') << setw(2) << endTime.second << "]";
      if ( pConstraint != &PASS_CONSTRAINT && pConstraint != &FAIL_CONSTRAINT) {
        ss << "(" << pConstraint->getTitle() << ")";
      }
      return ss.str();
    }
  };

}
#endif
