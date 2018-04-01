#ifndef SHUNT_H
#define SHUNT_H

#include "ArduinoComponent.h"

class Shunt : public ArduinoComponent
{
  
  public:  

  typedef unsigned short RatedAmps;
  static const RatedAmps RATED_100_AMPS = 100, 
                         RATED_200_AMPS = 200;

  typedef unsigned char MilliVoltDrop;
  static const MilliVoltDrop MILLIVOLTS_50 = 50, 
                             MILLIVOLTS_75 = 75, 
                             MILLIVOLTS_100 = 100;

  typedef unsigned char Channel;
  static const Channel CHANNEL_A0 = 0, 
                       CHANNEL_A1 = 1, 
                       CHANNEL_A2 = 2, 
                       CHANNEL_A3 = 3,
                       DIFFERENTIAL_0_1 = 4, 
                       DIFFERENTIAL_2_3 = 5;

  const int16_t INVALID_RATED_MILLIVOLTS = 65532, INVALID_CHANNEL = 65533;

  Adafruit_ADS1115 ads;
  RatedAmps ratedAmps;
  MilliVoltDrop ratedMilliVolts;
  Channel channel;

  float amps; // cache so it can be shared with voltage meter to compute power

  Shunt(RatedAmps ratedAmps = RATED_200_AMPS,
        MilliVoltDrop ratedMilliVolts = MILLIVOLTS_75,
        Channel channel = DIFFERENTIAL_0_1 ) :
      ratedAmps(ratedAmps),
      ratedMilliVolts(ratedMilliVolts),
      channel(channel)
  {
  }
  
  virtual void setup()
  {
    // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 0.1875mV (default)
    // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 0.125mV
    // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 0.0625mV
    // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.03125mV
    // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.015625mV
    // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.0078125mV

    ads.setGain(GAIN_SIXTEEN);
    ads.begin();
  }
  
  virtual int16_t readADC()
  {
    int16_t adc;
    if ( channel >= CHANNEL_A0 && channel <= CHANNEL_A3 )
    {
      adc = ads.readADC_SingleEnded(channel);
    }
    else if ( channel == DIFFERENTIAL_0_1 ) 
    { 
      adc = ads.readADC_Differential_0_1();  
    } 
    else if ( channel == DIFFERENTIAL_2_3 ) 
    {
      adc = ads.readADC_Differential_2_3();
    }
    else
    {
      adc = INVALID_CHANNEL;
    }
    return adc;
  }


  virtual float readAndCacheAmps() 
  {  
    // amps used later to compute watts from PowerMeter

    // amps = sample(this,&Shunt::readAmps);
    amps = readAmps();
    return amps;
  }
  
  float readAmps() 
  {
    int16_t shuntADC = readADC();
    float rtnAmps = 0;
    
    if ( shuntADC == -1 )
    {
      gLastErrorMsg = F("Shunt::readAmps() ADS1115 returned -1.  Check connections and pin mappings.");
    }
    else if ( shuntADC == INVALID_CHANNEL )
    {
      gLastErrorMsg = F("Shunt::readAmps() invalid ADS1115 channel: ");
      gLastErrorMsg += channel;
    }
    else 
    {
      // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
      float multiplier = ((float) ratedAmps) / ratedMilliVolts;
      float voltageDrop = ((float) shuntADC * 256.0) / 32768.0;
      rtnAmps = multiplier * voltageDrop;
    }
    return rtnAmps;
  }

  virtual void print(int depth = 0)
  {
    print(depth,false);
  }

  virtual void print(int depth, bool sameLine)
  {
    readAndCacheAmps();

    JsonSerialWriter w(depth);
    if ( sameLine )
    {
      w.noPrefixPrintln("{");
    }
    else
    {
      w.println("{");
    }
    w.increaseDepth();
    w.printlnNumberObj(F("amps"),amps,",");
    w.printlnNumberObj(F("ratedAmps"),ratedAmps,",");
    w.printlnNumberObj(F("ratedMilliVolts"),ratedMilliVolts);
    w.decreaseDepth();
    w.print("}");
  }

};

#endif