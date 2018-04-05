#ifndef POWERMETERCONFIG_H
#define POWERMETERCONFIG_H

class PowerMeterConfig
{
  public:

  static size_t getElementSize() { return 3*sizeof(float); }
  
  static size_t computeFirstElementAddr( Device** devices ) {
      int deviceCnt = 0;
      for( int i = 0; devices[i] != NULL; i++ ){ deviceCnt++; }
      return DeviceConfig::getFirstElementAddr() + deviceCnt * DeviceConfig::getElementSize();
  }
  
  int index;
  int firstPowerMeterAddr;
  PowerMeter* pPowerMeter;

  float voltmeterVcc;
  float r1;
  float r2;

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
    addr += sizeof(float);
    EEPROM.get( addr, r1 );
    pPowerMeter->voltmeter->r1 = r1;
    addr += sizeof(float);
    EEPROM.get( addr, r2 );
    pPowerMeter->voltmeter->r2 = r2;
  }

  void save()
  {
    int addr = firstPowerMeterAddr + index * PowerMeterConfig::getElementSize();
    EEPROM.put( addr, pPowerMeter->voltmeter->vcc );
    addr += sizeof(float);
    EEPROM.put( addr, pPowerMeter->voltmeter->r1 );
    addr += sizeof(float);
    EEPROM.put( addr, pPowerMeter->voltmeter->r2 );
  }
  
};


#endif