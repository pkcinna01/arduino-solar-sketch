
#ifndef ARDUINO_DHT_TEMP_SENSOR_H
#define ARDUINO_DHT_TEMP_SENSOR_H

#include "DhtSensor.h"

class DhtTempSensor : public DhtSensor {
  public:
  DhtTempSensor(const char* const name, Dht& dht)
  : DhtSensor(name,dht)
  {    
  }

  float getValueImpl() const override {    
    if ( isCacheExpired() || status.code != 0 ) {
      cacheValue(dht.readTemp());
      status = dht.status;
    } 
    return cachedValue;
  }
};

#endif