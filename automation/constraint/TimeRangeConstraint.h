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

  class TimeRangeConstraint : public Constraint {
  public:
    RTTI_GET_TYPE_IMPL(automation,TimeRange)
    
    struct Time {
      int hour, minute, second;
    } beginTime, endTime;

    TimeRangeConstraint(Time beginTime, Time endTime) :
        beginTime(beginTime),
        endTime(endTime) {
    }

    bool checkValue() override {
      if ( !automation::isTimeValid() ) {
        return false; // for arduino when no time hardware and time never set
      }
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

    string getTitle() const override {
      stringstream ss;
      ss << getType() << "[" << setfill('0') << setw(2) << beginTime.hour << ":"
         << setfill('0') << setw(2) << beginTime.minute << ":"
         << setfill('0') << setw(2) << beginTime.second
         << "-"
         << setfill('0') << setw(2) << endTime.hour << ":"
         << setfill('0') << setw(2) << endTime.minute << ":"
         << setfill('0') << setw(2) << endTime.second << "]";      
      return ss.str();
    }
  };

}
#endif
