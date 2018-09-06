
#ifndef ARDUINO_THERMISTOR_SENSOR_H
#define ARDUINO_THERMISTOR_SENSOR_H

#include "AnalogSensor.h"

class ThermistorSensor : public AnalogSensor {
  public:
  float beta; //3950.0,  3435.0 
  float balanceResistance, roomTempResistance, roomTempKelvin;
    
  ThermistorSensor(const char* const name,
             int sensorPin, 
             float beta = 3950, 
             float balanceResistance = 9999.0, 
             float roomTempResistance = 10000.0,
             float roomTempKelvin = 298.15):
    AnalogSensor(name,sensorPin,10,25),
    beta(beta),
    balanceResistance(balanceResistance),
    roomTempResistance(roomTempResistance),
    roomTempKelvin(roomTempKelvin)
  {
  }

  float getValueImpl() const override {
    return this->readBetaCalculatedTemp();
  }

  virtual float readBetaCalculatedTemp() const
  {
    int pinVoltage = analogRead(sensorPin);
    float rThermistor = balanceResistance * ( (1023.0 / pinVoltage) - 1);
    float tKelvin = (beta * roomTempKelvin) / 
            (beta + (roomTempKelvin * log(rThermistor / roomTempResistance)));

    float tCelsius = tKelvin - 273.15;
    float tFahrenheit = (tCelsius * 9.0)/ 5.0 + 32.0;
    return tFahrenheit;
  }
};

#endif