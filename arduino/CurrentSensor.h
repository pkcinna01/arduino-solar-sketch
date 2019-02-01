
#ifndef ARDUINO_CURRRENT_SENSOR_H
#define ARDUINO_CURRRENT_SENSOR_H
#include "ArduinoSensor.h"
#include "../automation/json/JsonStreamWriter.h"

#include <Adafruit_ADS1015.h>

using namespace automation::json;

class CurrentSensor : public ArduinoSensor {
public:
  RTTI_GET_TYPE_IMPL(arduino,CurrentSensor)

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

  const int16_t INVALID_RATED_MILLIVOLTS = 65532, INVALID_CHANNEL = 65533, FAIL_RETURN_VALUE = 0;

  mutable Adafruit_ADS1115 ads {0x48};
  RatedAmps ratedAmps;
  MilliVoltDrop ratedMilliVolts;
  Channel channel;
  adsGain_t gain;

  CurrentSensor(const char *name,
                RatedAmps ratedAmps = RATED_200_AMPS,
                MilliVoltDrop ratedMilliVolts = MILLIVOLTS_75,
                Channel channel = /*CHANNEL_A0*/DIFFERENTIAL_0_1, adsGain_t gain = /*GAIN_EIGHT*/GAIN_SIXTEEN) :
      ArduinoSensor(name, 0 /*no pin*/),
      ratedAmps(ratedAmps),
      ratedMilliVolts(ratedMilliVolts),
      channel(channel),
      gain(gain) {
  }
 
  void setup() override {
    ads.setGain(gain);    
    ads.begin();
  }

  float getValueImpl() const override {
    float amps = readAmps();
    return amps;
  }

  virtual float readAmps() const {
    float rtnAmps = 0;

    int shuntADC = -1;
    for ( int i = 0; i < 3 && shuntADC < 0; i++ ) {
      shuntADC = readADC(50);
    }
    /*if (shuntADC < 0) {
      status.error( __PRETTY_FUNCTION__ );
      status.msg += F(" readADC() returned ");
      status.msg += shuntADC;
      status.msg += F(". Check connections and pin mappings.");
      rtnAmps = FAIL_RETURN_VALUE;
    } else*/
    if (shuntADC == INVALID_CHANNEL) {
      status.error( __PRETTY_FUNCTION__ );
      status.msg += F(" invalid ADS1115 channel ");
      status.msg += channel;
      rtnAmps = FAIL_RETURN_VALUE;
    } else {
      double milliVolts = shuntADC * getMilliVoltIncrement();
      rtnAmps = milliVolts / getRatedMilliOhms();
    }
    return rtnAmps;
  }

  virtual int16_t readADC(int delayMs = 0) const {
    delay(delayMs);

    int16_t adc;
    if (channel >= CHANNEL_A0 && channel <= CHANNEL_A3) {
      adc = ads.readADC_SingleEnded(channel);
    } else if (channel == DIFFERENTIAL_0_1) {
      adc = ads.readADC_Differential_0_1();
      //cout << __PRETTY_FUNCTION__ << " readADC_Differential_0_1=" << adc << endl;
    } else if (channel == DIFFERENTIAL_2_3) {
      adc = ads.readADC_Differential_2_3();
    } else {
      adc = INVALID_CHANNEL;
    }
    return adc;
  }

  double getRatedMilliOhms() const {
    double mv = ratedMilliVolts;
    double amps = ratedAmps;
    return mv / amps;
  }

  double getMilliVoltIncrement() const {
    // GAIN_TWOTHIRDS 2/3x gain  +/- 6.144V  1 bit = 0.1875mV (default)
    // GAIN_ONE        1x gain   +/- 4.096V  1 bit = 0.125mV
    // GAIN_TWO        2x gain   +/- 2.048V  1 bit = 0.0625mV
    // GAIN_FOUR       4x gain   +/- 1.024V  1 bit = 0.03125mV
    // GAIN_EIGHT      8x gain   +/- 0.512V  1 bit = 0.015625mV
    // GAIN_SIXTEEN   16x gain   +/- 0.256V  1 bit = 0.0078125mV
    switch (gain) {
      case GAIN_ONE: return 0.125;
      case GAIN_TWO: return 0.0625;
      case GAIN_FOUR: return 0.03125;
      case GAIN_EIGHT: return 0.015625;
      case GAIN_SIXTEEN: return 0.0078125;
      case GAIN_TWOTHIRDS: 
      default: return 0.1875;
    }
  }

  void printVerboseExtra(JsonStreamWriter& w) const override {
    String strChannel;
    switch (channel) {
      case CHANNEL_A0: strChannel = F("CHANNEL_A0"); break;
      case CHANNEL_A1: strChannel = F("CHANNEL_A1"); break;
      case CHANNEL_A2: strChannel = F("CHANNEL_A2"); break;
      case CHANNEL_A3: strChannel = F("CHANNEL_A3"); break;
      case DIFFERENTIAL_0_1: strChannel = F("DIFFERENTIAL_0_1"); break;
      case DIFFERENTIAL_2_3: strChannel = F("DIFFERENTIAL_2_3"); break;
      default:
      strChannel = F("Invalid: ");
      strChannel += channel;
    }
    w.printlnStringObj(F("channel"), strChannel,",");
    int16_t shuntADC = readADC(50);
    w.printlnNumberObj(F("shuntADC"), shuntADC, ",");
    String strGain;
    switch (gain) {
      case GAIN_ONE: strGain = F("GAIN_ONE"); break;
      case GAIN_TWO: strGain = F("GAIN_TWO"); break;
      case GAIN_FOUR: strGain = F("GAIN_FOUR"); break;
      case GAIN_EIGHT: strGain = F("GAIN_EIGHT"); break;
      case GAIN_SIXTEEN: strGain = F("GAIN_SIXTEEN"); break;
      case GAIN_TWOTHIRDS: 
      default:  strGain = F("GAIN_TWOTHIRDS");
    };
    w.printlnStringObj(F("gain"), strGain, ",");
    char buffer[20];
    dtostrf(getMilliVoltIncrement(), 2, 8, buffer);
    w.printlnNumberObj(F("millivoltIncrement"),buffer,",");
    w.printlnNumberObj(F("ratedAmps"), ratedAmps, ",");
    w.printlnNumberObj(F("ratedMilliVolts"), ratedMilliVolts,",");
    
    double ohms = getRatedMilliOhms()/1000.0;
    dtostrf(ohms, 2, 8, buffer);
    w.printlnNumberObj(F("ratedResistance"), buffer,",");
  }

};

#endif