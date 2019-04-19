
#ifndef ARDUINO_DHT_TEMP_SENSOR_H
#define ARDUINO_DHT_TEMP_SENSOR_H

#include "DhtSensor.h"
namespace arduino {
class DhtTempSensor : public DhtSensor {
  public:
  DhtTempSensor(const char* const name, Dht& dht)
  : DhtSensor(name,dht)
  {    
  }

  RTTI_GET_TYPE_IMPL(arduino,DhtTempSensor)
 
  float getValueImpl() const override {    
    float rtnTemp = dht.readTemp();
    status = dht.status;
    return rtnTemp;
  }

};
}
#endif
