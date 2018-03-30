#ifndef VOLTMETER_H
#define VOLTMETER_H

#include "ArduinoComponent.h"

class Voltmeter : public ArduinoComponent
{
  public:
  
  int analogPin;
  float r1;
  float r2; // measured from analog pin to ground
  float vcc;
  float volts = 0; // cached for use with power meter
  
  Voltmeter(int analogPin, float r1 = 1000000.0, float r2 = 100000.0, float vcc = 5.0) : 
    analogPin(analogPin),
    r1(r1),
    r2(r2),
    vcc(vcc) 
  {    
  }

  virtual float readAndCacheVoltage() 
  {  
    // first read consistantly seems too high so ignore it
    readVoltage();
    
    // volts used later to compute watts from PowerMeter
    volts = sample(this,&Voltmeter::readVoltage, 10, 50);
    return volts;
  }

  float readVoltage()
  {
    float adc = analogRead(analogPin);
    float dividerWeight = ((float)(r1 + r2))/r2;
    return dividerWeight * adc * vcc / 1024.0;
  }

  virtual void print(int depth = 0)
  {
    print(depth,false);
  }

  virtual void print(int depth, bool sameLine)
  {
    readAndCacheVoltage();
    float assignedVcc = vcc;
    float assignedR1 = r1;
    float assignedR2 = r2;

    JsonSerialWriter w(depth);
    if ( sameLine )
    {
      w.appendPrintln("{");
    }
    else
    {
      w.println("{");
    }
    w.increaseDepth();
    w.printlnNumberObj(F("volts"),volts,",");
    w.printlnNumberObj(F("analogPin"),analogPin,",");
    w.printlnNumberObj(F("assignedVcc"),assignedVcc,",");
    w.printlnNumberObj(F("assignedR1"),assignedR1,",");
    w.printlnNumberObj(F("assignedR2"),assignedR2);
    w.decreaseDepth();
    w.print("}");
  }
};

#endif