
#ifndef ARDUINO_DHT_HUMIDITY_SENSOR_H
#define ARDUINO_DHT_HUMIDITY_SENSOR_H

#include "DhtSensor.h"

class DhtHumiditySensor : public DhtSensor {
  public:
  DhtHumiditySensor(const char* const name, Dht& dht)
  : DhtSensor(name,dht)
  {    
  }

  float getValueImpl() const override {
    if ( isCacheExpired() ) {
      cacheValue(dht.readHumidity());
    } 
    return cachedValue;
  }
};

#endif