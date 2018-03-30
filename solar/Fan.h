#ifndef FAN_H
#define FAN_H

#include "ArduinoComponent.h"

class Fan : public ArduinoComponent
{
  public:
  const char* const name;
  float onTemp;
  float offTemp;
  int relayPin;
  bool onValue = HIGH; // some relays are on when signal is set low/false instead of high/true

  static String getFanModeText(FanMode mode, int depth = 1) 
  {
    return fanMode == FAN_MODE_AUTO ? "AUTO" 
         : fanMode == FAN_MODE_ON ? "ON" 
         : fanMode == FAN_MODE_OFF ? "OFF" 
         : "INVALID";                       
  }
  
  Fan(const char* const name, int relayPin, float onTemp = 125, float offTemp = 115, bool onValue = HIGH) :
    name(name),
    relayPin(relayPin),
    onTemp(onTemp),
    offTemp(offTemp),
    onValue(onValue)
  {     
  }

  virtual void setup()
  {
    pinMode(relayPin,OUTPUT);
    digitalWrite(relayPin, !onValue);
    #ifdef DEBUG
    Serial.print( "#'" ); Serial.print(name); Serial.print(F("' fan (relayPin="));
    Serial.print(relayPin); Serial.println(F(") mode set to OUTPUT"));
    #endif
  }

  virtual void update(float temp)
  {
    bool bTurnOn = false;
    bool bTurnOff = false;

    switch ( fanMode ) {
      case FAN_MODE_ON: 
        bTurnOn = true;
        break;
      case FAN_MODE_OFF: 
        bTurnOff = true; 
        break;
      case FAN_MODE_AUTO: 
        bTurnOn = temp >= onTemp;
        bTurnOff = temp < offTemp;
        break;
    }

    int relayPinVal = bitRead(PORTD,relayPin);
    bool bRelayOn = (relayPinVal == onValue);
    
    if ( bRelayOn && bTurnOff ) {
      digitalWrite(relayPin,!onValue);
    } else if ( !bRelayOn && bTurnOn ) {
      digitalWrite(relayPin,onValue);
    }
  }

  virtual void print(int depth = 0)
  {
    JsonSerialWriter w(depth);
    w.println("{");
    w.increaseDepth();
    w.printlnStringObj(F("name"),name,",");
    w.printlnNumberObj(F("relayPin"),relayPin,",");
    w.printlnNumberObj(F("onTemp"),onTemp,",");
    int relayValue = bitRead(PORTD,relayPin);
    w.printlnNumberObj(F("relayValue"),relayValue,",");
    w.printlnBoolObj(F("on"),relayValue == onValue);
    w.decreaseDepth();
    w.print("}");
  }
};

#endif