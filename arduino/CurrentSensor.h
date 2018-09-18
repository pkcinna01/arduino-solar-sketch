
#ifndef ARDUINO_CURRRENT_SENSOR_H
#define ARDUINO_CURRRENT_SENSOR_H
#include "ArduinoSensor.h"
#include "JsonWriter.h"

#include <Adafruit_ADS1015.h>

class CurrentSensor : public ArduinoSensor {
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

  Adafruit_ADS1115 ads {0x48};
  RatedAmps ratedAmps;
  MilliVoltDrop ratedMilliVolts;
  Channel channel;

  CurrentSensor(const char *name,
                RatedAmps ratedAmps = RATED_200_AMPS,
                MilliVoltDrop ratedMilliVolts = MILLIVOLTS_75,
                Channel channel = DIFFERENTIAL_0_1) :
      ArduinoSensor(name, 0 /*no pin*/),
      ratedAmps(ratedAmps),
      ratedMilliVolts(ratedMilliVolts),
      channel(channel) {
  }

  void setup() override {
    // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 0.1875mV (default)
    // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 0.125mV
    // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 0.0625mV
    // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.03125mV
    // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.015625mV
    // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.0078125mV

    ads.setGain(GAIN_SIXTEEN);
    ads.begin();
  }

  float getValueImpl() const override {
    float amps = readAmps();
    return amps;
  }

  virtual float readAmps() const {
    int16_t shuntADC = readADC();
    float rtnAmps = 0;

    if (shuntADC < 0
        && (shuntADC = readADC(50)) < 0
        && (shuntADC = readADC(50)) < 0) {
      // tried 3 times and all attempts failed
      gLastErrorMsg = F("Shunt::readAmps() ADS1115::readADC() returned ");
      gLastErrorMsg += shuntADC;
      gLastErrorMsg += F(". Check connections and pin mappings.");
      //cout << __PRETTY_FUNCTION__ << " Error:" << gLastErrorMsg << endl;
    } else if (shuntADC == INVALID_CHANNEL) {
      gLastErrorMsg = F("Shunt::readAmps() invalid ADS1115 channel: ");
      gLastErrorMsg += channel;
      //cout << __PRETTY_FUNCTION__ << " Error:" << gLastErrorMsg << endl;
    } else {
      // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
      float multiplier = ((float) ratedAmps) / ratedMilliVolts;
      float voltageDrop = ((float) shuntADC * 256.0) / 32768.0;
      //cout << __PRETTY_FUNCTION__ << " voltage drop from shuntADC: " << voltageDrop << endl;
      rtnAmps = multiplier * voltageDrop;
    }
    return rtnAmps;
  }

  virtual int16_t readADC(int delayMs = 0) const {
    //cout << __PRETTY_FUNCTION__ << " begin" << endl;
    //cout << "ads.readADC_SingleEnded(0)=" << ads.readADC_SingleEnded(0) << endl;
    delay(delayMs);

    int16_t adc;
    Adafruit_ADS1115* pAds = (Adafruit_ADS1115*) &ads;
    if (channel >= CHANNEL_A0 && channel <= CHANNEL_A3) {
      adc = pAds->readADC_SingleEnded(channel);
    } else if (channel == DIFFERENTIAL_0_1) {
      adc = pAds->readADC_Differential_0_1();
      //cout << __PRETTY_FUNCTION__ << " adc=" << adc << endl;
    } else if (channel == DIFFERENTIAL_2_3) {
      adc = pAds->readADC_Differential_2_3();
    } else {
      adc = INVALID_CHANNEL;
    }
    return adc;
  }

  void printVerbose(int depth) override {
    JsonSerialWriter w(depth);

    w.println("{");
    w.increaseDepth();
    w.printlnStringObj(F("name"), name.c_str(), ",");
    w.printlnNumberObj(F("value"), getValue(), ",");
    w.printlnNumberObj(F("ratedAmps"), ratedAmps, ",");
    w.printlnNumberObj(F("ratedMilliVolts"), ratedMilliVolts);
    w.decreaseDepth();
    w.print("}");
  }

};

#endif