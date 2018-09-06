
#ifndef ARDUINO_DHT_SENSOR_H
#define ARDUINO_DHT_SENSOR_H

#include "ArduinoSensor.h"
#include "Dht.h"

class DhtSensor : public ArduinoSensor {
  public:
  
  Dht& dht;
  
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
  
};

#endif