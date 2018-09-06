#ifndef ARDUINO_DHT_H
#define ARDUINO_DHT_H

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
    return readTemperature(FAHRENHEIT);
  }

  virtual float readHumidity()
  {
    return DHT::readHumidity();
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