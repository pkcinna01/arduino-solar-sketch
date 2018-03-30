#ifndef DEVICECONFIG_H
#define DEVICECONFIG_H


class DeviceConfig
{
  public:
  int index;
  Device* pDevice;

  // persistent fields (up 5 fans)
  static const unsigned int MAX_FAN_CNT = 4;

  float fanOnTemps[MAX_FAN_CNT];
  float fanOffTemps[MAX_FAN_CNT];

  static size_t getFirstElementAddr() { return sizeof(VERSION) + sizeof(fanMode) + sizeof(outputFormat); }
  static size_t getElementSize() { return MAX_FAN_CNT*2*sizeof(float); }
  
  DeviceConfig(int index, Device* pDevice) :
    index(index),
    pDevice(pDevice)
  {
  }

  void load()
  {
    int addr = DeviceConfig::getFirstElementAddr() + index * DeviceConfig::getElementSize();
    EEPROM.get( addr, fanOnTemps );
    EEPROM.get( addr + sizeof(fanOnTemps), fanOffTemps );
    for ( int i = 0; pDevice->fans[i] != NULL; i++ ) {
      Fan* pFan = pDevice->fans[i];
      pFan->onTemp = fanOnTemps[i];
      pFan->offTemp = fanOffTemps[i];
    }
  }

  void save()
  {
    int addr = DeviceConfig::getFirstElementAddr() + index * DeviceConfig::getElementSize();
    memset(fanOnTemps, 0, sizeof(fanOnTemps));
    memset(fanOffTemps, 0, sizeof(fanOffTemps));
    for ( int i = 0; pDevice->fans[i] != NULL; i++ )
    {
      Fan* pFan = pDevice->fans[i];
      fanOnTemps[i] = pFan->onTemp;
      fanOffTemps[i] = pFan->offTemp;
    }
    EEPROM.put( addr, fanOnTemps );
    EEPROM.put( addr + sizeof(fanOnTemps), fanOffTemps );
  }
  
};

#endif