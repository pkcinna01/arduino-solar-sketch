#ifndef POWERMETER_H
#define POWERMETER_H

#include "ArduinoComponent.h"

class PowerMeter : public ArduinoComponent
{
  public:
  const char* const name;
  Voltmeter* voltmeter;
  Shunt* shunt;

  PowerMeter(const char* const name, Voltmeter* voltmeter, Shunt* shunt):
    name(name),
    voltmeter(voltmeter),
    shunt(shunt)
  {      
  }

  virtual void setup()
  {
    shunt->setup();
  }

  virtual void print(int depth = 0)
  {
    JsonSerialWriter w(depth);
    w.println("{");
    w.increaseDepth();
    w.printlnStringObj(F("name"),name,",");
    w.printKey(F("voltage"));
    voltmeter->print(depth+1,true);
    w.appendPrintln(",");
    w.printKey(F("current"));
    shunt->print(depth+1,true);
    w.appendPrintln(",");
    // voltmeter volts and shunt amps values will be cached after print calls above
    float watts = voltmeter->volts * shunt->amps;
    w.printlnNumberObj(F("watts"),watts);
    w.decreaseDepth();
    w.print("}");
  }
};

#endif