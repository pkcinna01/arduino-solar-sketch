
#ifndef ARDUINO_DHT_SENSOR_H
#define ARDUINO_DHT_SENSOR_H

#include "ArduinoSensor.h"
#include "Dht.h"

class DhtSensor : public ArduinoSensor {
  public:
  
  Dht& dht;
  
  DhtSensor(const char* const name, Dht& dht) :
    ArduinoSensor(name,dht.sensorPin,1),
    dht(dht)
  {    
  }

  void setup() override {
    if ( !isInitialized() ) {
      dht.begin();
      setInitialized(true);
    }
  }
  
};

#endif
