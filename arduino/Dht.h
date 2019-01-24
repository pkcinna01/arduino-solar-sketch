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
      status.error( String(__PRETTY_FUNCTION__) + " returned NaN after " + RETRY_COUNT + " tries");
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
      status.error( String(__PRETTY_FUNCTION__) + " returned NaN after " + RETRY_COUNT + " tries");
      rtn = FAIL_VALUE;
    }
    return rtn;
  }

/*
  virtual void print(int depth = 1)
  {
    float temp = readTemp();
    float humidity = readHumidity();
    float heatIndex = computeHeatIndex(temp, humidity, FAHRENHEIT);
    JsonSerialWriter w(depth);
    w.println("{")
      .increaseDepth()
      .printlnNumberObj(F("temp"),temp,",")
      .printlnNumberObj(F("humidity"),humidity,",");
    if ( status.code == 0 ) {
      w.printlnNumberObj(F("heatIndex"),heatIndex);
    } else {
      w.printlnNumberObj(F("heatIndex"),heatIndex,",");
      w.printKey(F("status"));
      w.noPrefixPrintln("");
      status.print(depth+1);
    }
    w.printlnNumberObj(F("heatIndex"),heatIndex)
     .decreaseDepth()
     .print("}");
  }
  */
};
#endif