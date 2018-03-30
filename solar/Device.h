#ifndef DEVICE_H
#define DEVICE_H

#include "ArduinoComponent.h"

class Device : public ArduinoComponent
{
  public:
  const char* const name;
  Fan** fans;
  TempSensor** tempSensors;

  
  Device(const char* const name, Fan** fans, TempSensor** tempSensors) :
    name(name),
    fans(fans),
    tempSensors(tempSensors)
  {
  }

  virtual void setup()
  {
    for( int i = 0; tempSensors[i] != NULL; i++ )
    {
      tempSensors[i]->setup();
    }    
    for( int i = 0; fans[i] != NULL; i++ )
    {
      fans[i]->setup();
    }
  }

  virtual void update() 
  {
    float maxTemp = 0;
    for( int i = 0; tempSensors[i] != NULL; i++ )
    {
      float temp = tempSensors[i]->readTemp();
      if ( temp > maxTemp ) {
        maxTemp = temp;
      }
    }
    // enable/disable all fans for this device based on highest temp sensor value
    for( int i = 0; fans[i] != NULL; i++ )
    {
      fans[i]->update(maxTemp);
    }
  }

  virtual void print(int depth = 0)
  {
    JsonSerialWriter(depth)
      .println("{")
      .increaseDepth()
      .printlnStringObj(F("name"),name, ",")
      .printlnArrayObj(F("tempSensors"), tempSensors, ",")
      .printlnArrayObj(F("fans"),fans)
      .decreaseDepth()
      .print("}");
  }
};


#endif