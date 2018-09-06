#ifndef SOLAR_IFTTT_VALUECONSTRAINT_H
#define SOLAR_IFTTT_VALUECONSTRAINT_H

#include "Constraint.h"

#include <string>

using namespace std;

namespace automation {

  template<typename ValueT, typename ValueSourceT>
  class ValueConstraint : public Constraint {

  public:
    ValueConstraint(ValueSourceT& valueSource) : valueSource(valueSource) {
    }

    bool checkValue() override {
      const ValueT &value = getValue();
      bool bCheckPassed = checkValue(value);
      return bCheckPassed;
    }

    virtual ValueT getValue() {
      return valueSource.getValue();
    }

    virtual bool checkValue(const ValueT &val) = 0;

  protected:
    ValueSourceT& valueSource;

  };

  template<typename ValueT, typename ValueSourceT>
  class RangeConstraint : public ValueConstraint<ValueT,ValueSourceT> {
  public:

    ValueT minVal, maxVal;

    RangeConstraint(ValueT minVal, ValueT maxVal, ValueSourceT &valueSource)
        : ValueConstraint<ValueT,ValueSourceT>(valueSource), minVal(minVal), maxVal(maxVal) {
    }

    bool checkValue(const ValueT &value) override {
      ValueT minVal = this->minVal;
      ValueT maxVal = this->maxVal;

      if (this->deferredTimeMs) {
        if (this->bTestResult) {
          minVal -= this->failMargin;
          maxVal += this->failMargin;
        } else {
          minVal += this->passMargin;
          maxVal -= this->passMargin;
        }
      }

      return value >= minVal && value <= maxVal;
    }

    string getTitle() override {
      string rtn(this->valueSource.name);
      rtn += " RANGE(";
      rtn += asString(minVal);
      rtn += ",";
      rtn += asString(maxVal);
      rtn += ")";
      return rtn;
    }
  };

  template<typename ValueT, typename ValueSourceT>
  class MaxConstraint : public ValueConstraint<ValueT,ValueSourceT> {
  public:

    ValueT maxVal;

    MaxConstraint(ValueT maxVal, ValueSourceT &valueSource)
        : ValueConstraint<ValueT,ValueSourceT>(valueSource), maxVal(maxVal) {
    }

    bool checkValue(const ValueT &value) override {
      ValueT maxVal = this->maxVal;

      if (this->deferredTimeMs) {
        if (this->bTestResult) {
          maxVal += this->failMargin;
        } else {
          maxVal -= this->passMargin;
        }
      }

      return value <= maxVal;
    }

    string getTitle() override {
      string rtn(this->valueSource.name);
      rtn += " MAX(";
      rtn += asString(maxVal);
      rtn += ")";
      return rtn;
    }

  };

  template<typename ValueT, typename ValueSourceT>
  class MinConstraint : public ValueConstraint<ValueT,ValueSourceT> {
  public:

    ValueT minVal;

    MinConstraint(ValueT minVal, ValueSourceT &valueSource)
        : ValueConstraint<ValueT,ValueSourceT>(valueSource), minVal(minVal) {
    }

    bool checkValue(const ValueT &value) override {
      ValueT minVal = this->minVal;

      if (this->deferredTimeMs) {
        if (this->bTestResult) {
          minVal -= this->failMargin;
        } else {
          minVal += this->passMargin;
        }
      }
      return value >= minVal;
    }

    string getTitle() override {
      string rtn(this->valueSource.name);
      rtn += " MIN(";
      rtn += asString(minVal);
      rtn += ")";
      return rtn;
    }

  };

}
#endif //SOLAR_IFTTT_VALUECONSTRAINT_H
