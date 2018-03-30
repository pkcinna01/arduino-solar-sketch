#ifndef TEMPSENSOR_H
#define TEMPSENSOR_H

#include "ArduinoComponent.h"

class TempSensor : public ArduinoComponent
{
  public:
  const char* const name;
  int sensorPin;

  float beta; //3950.0,  3435.0 
  float balanceResistance, roomTempResistance, roomTempKelvin;
    
  TempSensor(const char* const name,
             int sensorPin, 
             float beta = 3950, 
             float balanceResistance = 9999.0, 
             float roomTempResistance = 10000.0,
             float roomTempKelvin = 298.15):
    name(name),
    sensorPin(sensorPin),
    beta(beta),
    balanceResistance(balanceResistance),
    roomTempResistance(roomTempResistance),
    roomTempKelvin(roomTempKelvin)
  {
  }

  virtual void setup()
  {
    pinMode(sensorPin,INPUT);
    #ifdef DEBUG
    Serial.print(F("#  '")); Serial.print(name); Serial.print(F("' thermistor analog input pin:")); Serial.println(sensorPin);
    #endif
  }
  
  virtual float readTemp() 
  {  
    return sample(this, &TempSensor::readBetaCalculatedTemp, 10, 25);
  }


  float readBetaCalculatedTemp()
  {
    int pinVoltage = analogRead(sensorPin);
    float rThermistor = balanceResistance * ( (1023.0 / pinVoltage) - 1);
    float tKelvin = (beta * roomTempKelvin) / 
            (beta + (roomTempKelvin * log(rThermistor / roomTempResistance)));

    float tCelsius = tKelvin - 273.15;
    float tFahrenheit = (tCelsius * 9.0)/ 5.0 + 32.0;
    return tFahrenheit;
  }
  
  virtual void print(int depth = 0)
  {
    float temp = readTemp();
    JsonSerialWriter w(depth);
    w.println("{");
    w.increaseDepth();
    w.printlnStringObj(F("name"),name,",");
    w.printlnNumberObj(F("temp"),temp);
    w.decreaseDepth();
    w.print("}");
  }
};




#endif