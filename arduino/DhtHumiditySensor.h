
#ifndef ARDUINO_DHT_HUMIDITY_SENSOR_H
#define ARDUINO_DHT_HUMIDITY_SENSOR_H

#include "DhtSensor.h"
namespace arduino {
class DhtHumiditySensor : public DhtSensor {
  public:
  DhtHumiditySensor(const char* const name, Dht& dht)
  : DhtSensor(name,dht)
  {    
  }

  RTTI_GET_TYPE_IMPL(arduino,DhtHumiditySensor)
 
  float getValueImpl() const override {
    if ( isCacheExpired() ) {
      cacheValue(dht.readHumidity());
    } 
    return cachedValue;
  }
};
}
#endif