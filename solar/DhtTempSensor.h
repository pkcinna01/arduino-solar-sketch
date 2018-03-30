#ifndef DHTTEMPSENSOR_H
#define DHTTEMPSENSOR_H

#include "TempSensor.h"

class DhtTempSensor : public TempSensor
{

  public:
  int dhtType;
  DHT dht;
  
  DhtTempSensor(const char* const name, int sensorPin, int dhtType = DHT22) :
    TempSensor(name,sensorPin),
    dhtType(dhtType),
    dht(sensorPin,dhtType)
  {    
  }

  virtual void setup()
  {
    dht.begin();
    #ifdef DEBUG
    Serial.print(F("#DHT")); Serial.print(dhtType); Serial.print(F(" started on digital pin ")); Serial.println(sensorPin);
    #endif
  }

  virtual float readHumidity()
  {
    return dht.readHumidity();
  }
  
  virtual float readTemp()
  {
    return dht.readTemperature(FAHRENHEIT);
  }

  virtual void print(int depth = 1)
  {
    float temp = readTemp();
    float humidity = readHumidity();
    float heatIndex = dht.computeHeatIndex(temp, humidity, FAHRENHEIT);
    JsonSerialWriter(depth)
      .println("{")
      .increaseDepth()
      .printlnStringObj(F("name"),name,",")
      .printlnNumberObj(F("temp"),temp,",")
      .printlnNumberObj(F("humidity"),humidity,",")
      .printlnNumberObj(F("heatIndex"),heatIndex)
      .decreaseDepth()
      .print("}");
  }
};




#endif