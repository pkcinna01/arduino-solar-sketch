#ifndef ARDUINO_DHT_H
#define ARDUINO_DHT_H

#include "Arduino.h"

#include <DHT.h>

#define FAHRENHEIT true

class Dht : public DHT {
  public:
  int sensorPin;
  bool bInitialized;
  
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
    float rtn = readTemperature(FAHRENHEIT);
    for (int i = 0; i < 4 && isnan(rtn); i++ ){
      automation::sleep(2100);
      rtn = readTemperature(FAHRENHEIT);      
      //cout << __PRETTY_FUNCTION__ << " result=" << rtn << " after NaN reading." << endl;
    }
    if ( isnan(rtn) ) {
      arduino::gLastErrorMsg = __PRETTY_FUNCTION__;
      arduino::gLastErrorMsg += " returned NaN after 4 tries";
    }
    return rtn;
  }

  virtual float readHumidity()
  {
    float rtn = DHT::readHumidity();
    for (int i = 0; i < 3 && isnan(rtn); i++ ){
      automation::sleep(2100);
      rtn = DHT::readHumidity();
    }
    if ( isnan(rtn) ) {
      arduino::gLastErrorMsg = __PRETTY_FUNCTION__;
      arduino::gLastErrorMsg += " returned NaN after 4 tries";
    }
    return rtn;
  }

  virtual void print(int depth = 1)
  {
    float temp = readTemp();
    float humidity = readHumidity();
    float heatIndex = computeHeatIndex(temp, humidity, FAHRENHEIT);
    JsonSerialWriter(depth)
      .println("{")
      .increaseDepth()
      .printlnNumberObj(F("temp"),temp,",")
      .printlnNumberObj(F("humidity"),humidity,",")
      .printlnNumberObj(F("heatIndex"),heatIndex)
      .decreaseDepth()
      .print("}");
  }
};
#endif