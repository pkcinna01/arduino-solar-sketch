#ifndef ARDUINO_DHT_H
#define ARDUINO_DHT_H

#include "Arduino.h"
#include "Status.h"

#include <DHT.h>

#define FAHRENHEIT true


class Dht : public DHT {
  public:
  int sensorPin;
  bool bInitialized;
  Status status;
  
  const int16_t RETRY_COUNT = 4, RETRY_WAIT_MS = 200, FAIL_VALUE = 0;

  Dht(int sensorPin, int dhtType = DHT22) 
  : DHT(sensorPin,dhtType),
    sensorPin(sensorPin)
  {  
  }

  virtual void begin() {
    if ( !bInitialized ) {
      bInitialized = true;
      DHT::begin();  
    }
  }

  virtual float readTemp() 
  {      
    status.reset();
    float rtn = readTemperature(FAHRENHEIT);
    for (int i = 0; i < RETRY_COUNT && isnan(rtn); i++ ){
      automation::sleep(RETRY_WAIT_MS);
      rtn = readTemperature(FAHRENHEIT);      
    }
    if ( isnan(rtn) ) {
      String errMsg = F("Dht::readTemp() returned NaN");
      status.error(errMsg) ;
      rtn = FAIL_VALUE;
    }
    return rtn;
  }

  virtual float readHumidity()
  {
    status.reset();
    float rtn = DHT::readHumidity();
    for (int i = 0; i < RETRY_COUNT && isnan(rtn); i++ ){
      automation::sleep(RETRY_WAIT_MS);
      rtn = DHT::readHumidity();
    }
    if ( isnan(rtn) ) {
      String errMsg = F("Dht::readHumidity() returned NaN");
      status.error(errMsg);
      rtn = FAIL_VALUE;
    }
    return rtn;
  }


};
#endif