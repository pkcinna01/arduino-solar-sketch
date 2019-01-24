#ifndef SOLAR_IFTTT_VALUECONSTRAINT_H
#define SOLAR_IFTTT_VALUECONSTRAINT_H

#include "Constraint.h"
#include "../text.h"

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
    RTTI_GET_TYPE_IMPL(automation,Range)
    ValueT minVal, maxVal;

    RangeConstraint(ValueT minVal, ValueT maxVal, ValueSourceT &valueSource)
        : ValueConstraint<ValueT,ValueSourceT>(valueSource), minVal(minVal), maxVal(maxVal) {
    }

    bool checkValue(const ValueT &value) override {
      ValueT minVal = this->minVal;
      ValueT maxVal = this->maxVal;

      if (this->deferredTimeMs) {
        if (this->isPassed()) {
          minVal -= this->failMargin;
          maxVal += this->failMargin;
        } else {
          minVal += this->passMargin;
          maxVal -= this->passMargin;
        }
      }

      return value >= minVal && value <= maxVal;
    }

    string getTitle() const override {
      string rtn(this->valueSource.name);
      rtn += " Range(";
      rtn += text::asString(minVal);
      rtn += ",";
      rtn += text::asString(maxVal);
      rtn += ")";
      return rtn;
    }
  };


  template<typename ValueT>
  class ConstantValueHolder : public ValueHolder<ValueT>  {
  public:
    ValueT val;
    
    ConstantValueHolder(ValueT val)
      : val(val) {
    }
    
    ValueT getValue() const override {
      return val;
    }
  };

  template<typename ValueT, typename ValueSourceT>
  class ThresholdValueConstraint : public ValueConstraint<ValueT,ValueSourceT> {
  public:
    RTTI_GET_TYPE_IMPL(automation,ThresholdValue)

    ValueHolder<ValueT>* pThreshold;

    bool bDeleteThreshold;

    ThresholdValueConstraint(ValueHolder<ValueT>& threshold, ValueSourceT &valueSource)
        : ValueConstraint<ValueT,ValueSourceT>(valueSource)
        , pThreshold(&threshold)
        , bDeleteThreshold(false) {
    }

    string getTitle() const override {
      string rtn(this->valueSource.name);
      rtn += " ";
      rtn += this->getType();
      rtn += "(";
      rtn += text::asString(pThreshold->getValue());
      rtn += ")";
      return rtn;
    }

    virtual ~ThresholdValueConstraint() {
      if ( bDeleteThreshold ) {        
        delete pThreshold;
      }
    }

    protected:
    ThresholdValueConstraint(ValueHolder<ValueT>* pThreshold, ValueSourceT &valueSource, bool bDeleteThreshold = true )
        : ValueConstraint<ValueT,ValueSourceT>(valueSource)
        , pThreshold(pThreshold)
        , bDeleteThreshold(bDeleteThreshold) {
    }

  };
  
  template<typename ValueT, typename ValueSourceT>
  class AtMost : public ThresholdValueConstraint<ValueT,ValueSourceT> {
  public:
    RTTI_GET_TYPE_IMPL(automation,AtMost)

    AtMost(ValueHolder<ValueT>& threshold, ValueSourceT &valueSource)
        : ThresholdValueConstraint<ValueT,ValueSourceT>(threshold,valueSource) {
    }

    AtMost(ValueT threshold, ValueSourceT &valueSource)
        : AtMost<ValueT,ValueSourceT>(new ConstantValueHolder<ValueT>(threshold),valueSource) {
    }

    bool checkValue(const ValueT &value) override {
      ValueT maxVal = this->pThreshold->getValue();

      if (this->deferredTimeMs) {
        if (this->isPassed()) {
          maxVal += this->failMargin;
        } else {
          maxVal -= this->passMargin;
        }
      }

      return value <= maxVal;
    }
  };

  template<typename ValueT, typename ValueSourceT>
  class AtLeast : public ThresholdValueConstraint<ValueT,ValueSourceT> {
  public:
    RTTI_GET_TYPE_IMPL(automation,AtLeast)

    AtLeast(ValueHolder<ValueT>& threshold, ValueSourceT &valueSource)
        : ThresholdValueConstraint<ValueT,ValueSourceT>(threshold,valueSource) {
    }

    AtLeast(ValueT threshold, ValueSourceT &valueSource)
        : ThresholdValueConstraint<ValueT,ValueSourceT>(new ConstantValueHolder<ValueT>(threshold),valueSource,true) {
    }

    bool checkValue(const ValueT &value) override {
      ValueT minVal = this->pThreshold->getValue();

      if (this->deferredTimeMs) {
        if (this->isPassed()) {
          minVal -= this->failMargin;
        } else {
          minVal += this->passMargin;
        }
      }
      return value >= minVal;
    }
  };

}
#endif //SOLAR_IFTTT_VALUECONSTRAINT_H
