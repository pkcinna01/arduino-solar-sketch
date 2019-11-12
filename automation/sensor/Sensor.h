#ifndef AUTOMATION_SENSOR_H
#define AUTOMATION_SENSOR_H

#include "../Automation.h"
#include "../json/JsonStreamWriter.h"
#include "../AttributeContainer.h"

#include <string>
#include <vector>
#include <algorithm>
#include <math.h>
#ifndef ARDUINO_APP
#include <memory>
#endif

namespace automation {


  template<typename ValueT>
  class ValueHolder {
  public:
    virtual ValueT getValue() const = 0;
    virtual ~ValueHolder() {

    }
  };

  class Sensor;

  class SensorListener {
    public:
    virtual void changed(const Sensor* pSensor, float newVal, float oldVal) const = 0;
  };

  class Sensor : public ValueHolder<float>, public NamedContainer {
  public:    
    RTTI_GET_TYPE_DECL;

    enum State { Undefined = 0, Initialized = 0x01, NotExpired = 0x02, NotSampleable = 0x04, NotCacheable = 0x08, Error = 0x0F };

    uint16_t sampleCnt;
    uint16_t sampleIntervalMs;
    
    #ifndef ARDUINO_APP
    std::unique_ptr<automation::SensorListener> pListener; // no space left for arduino to have listeners :-(
    #endif

    Sensor(const std::string& name, uint16_t sampleCnt=1, uint16_t sampleIntervalMs=35) : 
      NamedContainer(name),  
      state(State::Undefined), 
      cachedValue(0),
      sampleCnt(sampleCnt), 
      sampleIntervalMs(sampleIntervalMs)
    {
      assignId(this);
    }

    virtual void setup()
    {
      setInitialized(true);
    }

    virtual Sensor& reset()
    {
      setCacheExpired(true);
      setError(false);
      //cachedValue = 0;
      return *this;
    }

    void setInitialized(bool bInitialized) {
      if ( bInitialized ) {
        state |= State::Initialized;
      } else {
        state = state & ~State::Initialized;
      }
    }

    void setError(bool bError) {
      if ( bError ) {
        state |= State::Error;
      } else {
        state = state & ~State::Error;
      }
    }

    void setCanSample(bool bCanSample) {
      if ( bCanSample ) {
        state = state & ~State::NotSampleable;
      } else {
        state |= State::NotSampleable;
      }
    }

    void setCacheable(bool bCacheable) {
      if ( bCacheable ) {
        state = state & ~State::NotCacheable;
      } else {
        state |= State::NotCacheable;
      }
    }

    bool isInitialized() const { return (state & State::Initialized) > 0; }
    bool isError() const { return (state & State::Error) > 0; }
    bool isCacheExpired() const { return (state & State::NotExpired) == 0; }
    bool isCacheValid() const { return canCache() && !isCacheExpired(); }
    bool canCache() const { return ( state & State::NotCacheable ) == 0 ; }
    bool canSample() const { return ( state & State::NotSampleable) == 0; }

    float getValue() const override {

      if ( !isCacheValid() ) {     

        float val = sample(this, &Sensor::getValueImpl, sampleCnt, sampleIntervalMs);

        if ( canCache() ) {
          float old = cachedValue;
          setCachedValue(val);        

          #ifndef ARDUINO_APP
          if ( old != val && pListener ) {
            pListener->changed(this,val,old);
          }
          #endif
        }
      }
      return cachedValue;
    }
    
    virtual float getValueImpl() const = 0;

    virtual void print(json::JsonStreamWriter& w, bool bVerbose=false, bool bIncludePrefix=true) const override;

    // AVR chips dont have multi-threading but we can optimize sensor samplings by calling 
    // doSingleSample on all sensors.  By the time all sensors are sampled it may be close to
    // the time to do the next round of samples
    //
    // Returns false if sample was not done because sampleIntervalMs not elapsed
    //
    bool doSingleSample(int sampleIndex, Timer& lastSampleTimer) {
      if ( sampleIndex <= 1 ) {
        reset();
        cachedValue = getValueImpl();
      } else if ( lastSampleTimer.isExpiredMs(sampleIntervalMs) ) {
        lastSampleTimer.reset();
        cachedValue += getValueImpl(); // Note: the state of sensor is not "ValueCached" until all samples read and averaged
      } else {
        return false;
      }    
      if ( sampleIndex % sampleCnt == 0 ) {
        setCachedValue( cachedValue / sampleCnt );
      }
      return true;
    }

    template<typename ObjectPtr,typename MethodPtr>
    static float sample(ObjectPtr pObj, MethodPtr pMethod, unsigned int cnt = 5, unsigned int intervalMs = 50)
    {
      float sum = 0;
      if ( cnt == 0 ) {
        cnt = 1;
      }
      for (unsigned int i = 0; i < cnt; i++)
      {
        float sampleVal = (pObj->*pMethod)();

        if ( isnan(sampleVal) ) {
          return sampleVal;
        }
        sum += sampleVal;

        if ( cnt > 1 )
        {
          automation::sleep(intervalMs);
        }
      }
      return sum/cnt;
    }

    static bool compareValues(const Sensor* lhs, const Sensor* rhs);

    static float average(const vector<Sensor*>& sensors);
    static float minimum(const vector<Sensor*>& sensors);
    static float maximum(const vector<Sensor*>& sensors);
    static float delta(const vector<Sensor*>& sensors);

  protected:
    mutable unsigned char state = State::Undefined; // mutable because cached state can change even on a getValue()
    mutable float cachedValue;

    void setCacheExpired(bool bExpired) const {
      if ( !bExpired ) {
        state |= State::NotExpired;
      } else {
        state = state & ~State::NotExpired;
      }
    }

    void setCachedValue(float v) const {
      cachedValue = v;
      setCacheExpired(false);
    }
  };

  class SensorFn : public Sensor {
  public:
    RTTI_GET_TYPE_IMPL(automation::sensor,SensorFn);

    //const std::function<float()> getValueImpl;
    float (*getValueImplFn)(); // Arduino does not support function<> template

    SensorFn(const std::string& name, float (*getValueImplFn)())
        : Sensor(name), getValueImplFn(getValueImplFn)
    {
    }

    virtual float getValueImpl() const override
    {
      float rtn = getValueImplFn();
      return rtn;
    }

    float operator()() const {
      return getValue();
    }
  };


  // Use another sensor as the source for data but transform it based on a custom function
  class TransformSensor : public Sensor {
  public:
    Sensor& sourceSensor;
    float(*transformFn)(float);
    std::string transformName;

    TransformSensor(const std::string& transformName, Sensor& sourceSensor, float(*transformFn)(float)):
      Sensor( transformName + "(" + sourceSensor.name + ")"),
      sourceSensor(sourceSensor),
      transformFn(transformFn),
      transformName(transformName)
    {
    }
    
    virtual float getValue() const override
    {
      return transformFn(sourceSensor.getValue());
    }

  };

  struct SensorSampler {
    Sensor* pSensor;
    int sampleIndex;
    Timer lastSampleTimer;

    SensorSampler(Sensor* s):pSensor(s), sampleIndex(1){}

    void begin() {
      pSensor->reset(); // invalidate cache
    }

    bool doSingleSample() {
      if ( pSensor->doSingleSample(sampleIndex,lastSampleTimer) ) {
        sampleIndex++;
        return true;
      }
      return false;
    }

    bool isComplete() const {
      return pSensor->isCacheValid();
    }
  };

  class Sensors : public AttributeContainerVector<Sensor*> {
  public:
    Sensors(){}
    Sensors( vector<Sensor*>& sensors ) : AttributeContainerVector<Sensor*>(sensors) {}
    Sensors( vector<Sensor*> sensors ) : AttributeContainerVector<Sensor*>(sensors) {}
    
    void reset() {
      for( Sensor* pSensor : *this ) {
        pSensor->reset();
      }
    }

    void getValuesBySampling() 
    {
      std::vector<SensorSampler> sampleVec;
      
      for( Sensor* pSensor : *this ) {
        if ( pSensor->canSample() ) {
          sampleVec.push_back( SensorSampler(pSensor) );
        }
      }

      while( !sampleVec.empty() ) {
        for( SensorSampler& sampler : sampleVec) {
          sampler.doSingleSample();
        }
        sampleVec.erase( std::remove_if(sampleVec.begin(),sampleVec.end(), [](const SensorSampler& ss){return ss.isComplete();}), sampleVec.end());
      };
    }


  };

}

#endif
