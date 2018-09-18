
#ifndef ARDUINO_DHT_SENSOR_H
#define ARDUINO_DHT_SENSOR_H

#include "ArduinoSensor.h"
#include "Dht.h"

class DhtSensor : public ArduinoSensor {
  public:
  
  Dht& dht;
  
  mutable float cachedValue;

  mutable unsigned long lastReadTimeMs;

  DhtSensor(const char* const name, Dht& dht) :
    ArduinoSensor(name,dht.sensorPin),
    dht(dht)
  {    
  }

  void setup() override {
    if ( !bInitialized ) {
      dht.begin();
      bInitialized = true;
    }
  }

  bool isCacheExpired() const {
    return (automation::millisecs() - lastReadTimeMs) > 2100;
  }

  float cacheValue(float value) const {
    lastReadTimeMs = automation::millisecs();
    cachedValue = value;
    return cachedValue;
  }
  
};

#endif