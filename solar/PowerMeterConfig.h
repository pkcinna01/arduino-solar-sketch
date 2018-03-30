#ifndef POWERMETERCONFIG_H
#define POWERMETERCONFIG_H

class PowerMeterConfig
{
  public:

  static size_t getElementSize() { return sizeof(float); }
  
  static size_t computeFirstElementAddr( Device** devices ) {
      int deviceCnt = 0;
      for( int i = 0; devices[i] != NULL; i++ ){ deviceCnt++; }
      return DeviceConfig::getFirstElementAddr() + deviceCnt * DeviceConfig::getElementSize();
  }
  
  int index;
  int firstPowerMeterAddr;
  PowerMeter* pPowerMeter;

  float voltmeterVcc;

  PowerMeterConfig(int firstPowerMeterAddr, int index, PowerMeter* pPowerMeter) :
    firstPowerMeterAddr(firstPowerMeterAddr),
    index(index),
    pPowerMeter(pPowerMeter)
  {
  }

  void load()
  {
    int addr = firstPowerMeterAddr + index * PowerMeterConfig::getElementSize();    
    EEPROM.get( addr, voltmeterVcc );
    pPowerMeter->voltmeter->vcc = voltmeterVcc;
  }

  void save()
  {
    int addr = firstPowerMeterAddr + index * PowerMeterConfig::getElementSize();
    EEPROM.put( addr, pPowerMeter->voltmeter->vcc );
  }
  
};


#endif