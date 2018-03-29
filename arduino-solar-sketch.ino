//////////////////////////////////////////////////////////////////////////
//
// Project enables/disables fans based on temperature monitoring devices
// 
// A device can have multiple fans and multiple temperature guages.  All 
// fans for a device are turned on/off based on the max temperature probe
// reading of that device.
//
// This current setup controls 2 fans and monitors power usage
// 1) Ventilation fan on enclosure wall controlled with a single DHT temp sensor
// 2) An oscillating fan cooling 2 epsolar charge controllers where each has 
//    a dedicated thermistor temp sensor. 
//
// The serial bus support allows info about fans and temperatures to be
// monitored with Prometheus and displayed in Grafana.  The EPSolar
// charge controller metrics from monitoring can override the arduino fan
// regulation, A Grafana or Prometheus alert can trigger a 
// "SET_FAN_MODE,0N,TRANSIENT" command to temporarily take control.
// "SET_FAN_MODE,AUTO,TRANSIENT" will give control back to arduino temp sensors.
// Prometheus gets arduino metrics using the Java "arduino-client" in same
// parent folder as this project.
// 
//////////////////////////////////////////////////////////////////////////

#ifndef ARDUINO // HACK for intellisense in VSCODE
  #define ARDUINO 10805  
  #include <iom328p.h>
#endif

#include <DHT.h>
#include <EEPROM.h>
#include <string.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>

using namespace std;

#define VERSION_SIZE 14
#define FAHRENHEIT true

//
// Change this version if code changes impact data structures in EEPROM.
// Warning: Version change will reset EEPROM data to defaults
//
#define VERSION "SOLAR-1.4.0.1"

typedef unsigned char FanMode;

const FanMode FAN_MODE_OFF = 0, FAN_MODE_ON = 0x1, FAN_MODE_AUTO = 0x2;

FanMode fanMode = FAN_MODE_AUTO;

//#define DEBUG
//#define VERBOSE

#define JPRINT_STRING(s) {Serial.print("\""); Serial.print(F(#s)); Serial.print("\": \""); Serial.print(s); Serial.print("\"");}
#define JPRINT_NUMBER(n) {Serial.print("\""); Serial.print(F(#n)); Serial.print("\": "); Serial.print(n);}
#define JPRINT_BOOLEAN(b) {Serial.print("\""); Serial.print(F(#b)); Serial.print("\": "); Serial.print(b?"true":"false");}
#define JPRINT_KEY(k) {Serial.print("\""); Serial.print(F(#k)); Serial.print("\": ");}
#define JPRINT_ARRAY(arr) Serial.print("\""); Serial.print(F(#arr)); Serial.print("\": [\n    ");\
  for( int i = 0; arr[i] != NULL; i++ ){\
    if ( i != 0 )\
      Serial.print(",\n    ");\
    arr[i]->print();\
  };\
  Serial.print("\n    ]");


template<typename ObjectPtr,typename MethodPtr> float sample(ObjectPtr obj, MethodPtr method, unsigned int cnt = 5, unsigned int intervalMs = 50) 
{
  float sum = 0;
  for (int i = 0; i < cnt; i++)
  {
    sum += (obj->*method)();
    if ( cnt > 1 ) 
    {
      delay(intervalMs);
    }
  }
  return sum/cnt;
}

String gLastErrorMsg;
String gLastInfoMsg;

class Fan 
{
  public:
  const char* const name;
  float onTemp;
  float offTemp;
  int relayPin;
  bool onValue = HIGH; // some relays are on when signal is set low/false instead of high/true

  static void printFanMode(FanMode mode) 
  {
    String fanModeText = fanMode == FAN_MODE_AUTO ? "AUTO" 
                   : fanMode == FAN_MODE_ON ? "ON" 
                   : fanMode == FAN_MODE_OFF ? "OFF" 
                   : "INVALID";
    Serial.print("  "); 
    JPRINT_NUMBER(fanMode);
    Serial.print(", ");
    JPRINT_STRING(fanModeText);
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

  virtual void print()
  {
    Serial.print("  { ");
    JPRINT_STRING(name);
    Serial.print(", ");
    JPRINT_NUMBER(relayPin);
    Serial.print(", ");
    JPRINT_NUMBER(onTemp);
    Serial.print(", ");
    JPRINT_NUMBER(offTemp);
    Serial.print(", ");
    int relayValue = bitRead(PORTD,relayPin);
    JPRINT_NUMBER(relayValue);
    Serial.print(", ");
    bool on = relayValue == onValue;
    JPRINT_BOOLEAN(on);
    Serial.print(" }");
  }
};


class TempSensor 
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
  
  virtual void print()
  {
    float temp = readTemp();
    Serial.print("  { ");
    JPRINT_STRING(name);
    Serial.print(", ");
    JPRINT_NUMBER(temp);
    Serial.print(" }");
  }
};



class DhtTempSensor : public TempSensor
{

  public:
  int dhtType;
  DHT dht;
  
  DhtTempSensor(const char* const name, int sensorPin, int dhtType = DHT22) :
    TempSensor(name,sensorPin),
    dhtType(dhtType),
    dht(sensorPin,dhtType)
  {    
  }

  virtual void setup()
  {
    dht.begin();
    #ifdef DEBUG
    Serial.print(F("#DHT")); Serial.print(dhtType); Serial.print(F(" started on digital pin ")); Serial.println(sensorPin);
    #endif
  }

  virtual float readHumidity()
  {
    return dht.readHumidity();
  }
  
  virtual float readTemp()
  {
    return dht.readTemperature(FAHRENHEIT);
  }

  virtual void print()
  {
    float temp = readTemp();
    float humidity = readHumidity();
    float heatIndex = dht.computeHeatIndex(temp, humidity, FAHRENHEIT);
    Serial.print("  { ");
    JPRINT_STRING(name);
    Serial.print(", ");
    JPRINT_NUMBER(temp);
    Serial.print(", ");
    JPRINT_NUMBER(humidity);
    Serial.print(", ");
    JPRINT_NUMBER(heatIndex);
    Serial.print(" }");
  }
};


class Device 
{
  public:
  const char* const name;
  Fan** fans;
  TempSensor** tempSensors;

  static void printDevices(Device** devices)
  {
    Serial.print("  ");
    JPRINT_KEY(devices);
    Serial.print("[");
    for( int i = 0; devices[i] != NULL; i++ )
    {
      if ( i != 0 ) {
        Serial.print(",");
      }
      Serial.println();
      devices[i]->print();    
    }
    Serial.print("]");
  }
  
  Device(const char* const name, Fan** fans, TempSensor** tempSensors) :
    name(name),
    fans(fans),
    tempSensors(tempSensors)
  {
  }

  virtual void setup()
  {
    #ifdef DEBUG
    Serial.print("#  "); Serial.print(name); Serial.println(F(" setup() begin..."));
    #endif

    for( int i = 0; tempSensors[i] != NULL; i++ )
    {
      tempSensors[i]->setup();
    }    
    for( int i = 0; fans[i] != NULL; i++ )
    {
      fans[i]->setup();
    }

    #ifdef DEBUG
    Serial.print("#  "); Serial.print(name); Serial.println(F(" setup() complete."));
    #endif
  }

  virtual void update() 
  {
    float maxTemp = 0;
    for( int i = 0; tempSensors[i] != NULL; i++ )
    {
      float temp = tempSensors[i]->readTemp();
      if ( temp > maxTemp ) {
        maxTemp = temp;
      }
    }
    // enable/disable all fans for this device based on highest temp sensor value
    for( int i = 0; fans[i] != NULL; i++ )
    {
      fans[i]->update(maxTemp);
    }
  }

  virtual void print()
  {
    Serial.print("  { ");
    JPRINT_STRING(name);
    Serial.print(",\n    ");
    JPRINT_ARRAY(tempSensors);
    Serial.print(",\n    ");
    JPRINT_ARRAY(fans);
    Serial.println();
    Serial.print("  ");
    Serial.print("}");
  }
};


class Shunt
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
    //amps = sample(this,&Shunt::readAmps);
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

  virtual void print()
  {
    readAndCacheAmps();

    Serial.print("{ ");
    JPRINT_NUMBER(amps);
    Serial.print(", ");
    JPRINT_NUMBER(ratedAmps);
    Serial.print(", ");
    JPRINT_NUMBER(ratedMilliVolts);
    Serial.print(" }");
  }

};


class Voltmeter
{
  public:
  
  int analogPin;
  float r1;
  float r2; // measured from analog pin to ground
  float vcc;
  float volts = 0; // cached for use with power meter
  
  Voltmeter(int analogPin, float r1 = 1000000.0, float r2 = 100000.0, float vcc = 5.0) : 
    analogPin(analogPin),
    r1(r1),
    r2(r2),
    vcc(vcc) 
  {    
  }

  virtual float readAndCacheVoltage() 
  {  
    // first read consistantly seems too high so ignore it
    readVoltage();
    
    // volts used later to compute watts from PowerMeter
    volts = sample(this,&Voltmeter::readVoltage, 10, 50);
    return volts;
  }

  float readVoltage()
  {
    float adc = analogRead(analogPin);
    float dividerWeight = ((float)(r1 + r2))/r2;
    return dividerWeight * adc * vcc / 1024.0;
  }

  virtual void print()
  {
    readAndCacheVoltage();
    float assignedVcc = vcc;
    float assignedR1 = r1;
    float assignedR2 = r2;
    Serial.print("{ ");
    JPRINT_NUMBER(volts);
    Serial.print(", ");
    JPRINT_NUMBER(analogPin);
    Serial.print(", ");
    JPRINT_NUMBER(assignedVcc);
    Serial.print(", ");
    JPRINT_NUMBER(assignedR1);
    Serial.print(", ");
    JPRINT_NUMBER(assignedR2);
    Serial.print(" }");
  }
};


class PowerMeter
{
  public:
  const char* const name;
  Voltmeter* voltmeter;
  Shunt* shunt;

  static void printPowerMeters(PowerMeter** power)
  {
    Serial.print("  ");
    JPRINT_KEY(powerMeters);
    Serial.print("[");
    for( int i = 0; power[i] != NULL; i++ )
    {
      if ( i != 0 ) {
        Serial.print(",");
      }
      Serial.println();
      power[i]->print();    
    }
    Serial.print("]");
  }

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

  virtual void print()
  {
    Serial.print("  {\n    ");
    JPRINT_STRING(name);
    Serial.print(",\n    ");
    JPRINT_KEY(voltage);
    voltmeter->print();
    Serial.print(",\n    ");
    JPRINT_KEY(current);
    shunt->print();
    Serial.print(",\n    ");
    // voltmeter volts and shunt amps values will be cached after print calls above
    float watts = voltmeter->volts * shunt->amps;
    JPRINT_NUMBER(watts);
    Serial.print("\n  }");
  }
};


class DeviceConfig
{
  public:
  int index;
  Device* pDevice;

  // persistent fields (up 5 fans)
  static const unsigned int MAX_FAN_CNT = 4;

  float fanOnTemps[MAX_FAN_CNT];
  float fanOffTemps[MAX_FAN_CNT];

  static size_t getFirstElementAddr() { return sizeof(VERSION) + sizeof(fanMode); }
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

/////////////////////////////////////////////////////////////////////////
//
// Declarations
//
/////////////////////////////////////////////////////////////////////////

//
// Fan relays used are enabled with LOW signal but we are wiring to "normally closed" 
// instead of "normally open".  This should run fans when usb power or arduino board is down
//
Fan exhaustFan("Exhaust",3,90,85,HIGH); 
Fan oscillatingFan("Oscillating",2,115,100,HIGH);

DhtTempSensor benchTempSensor("DHT",7);
TempSensor tempSensor1("Thermistor1",0);
TempSensor tempSensor2("Thermistor2",1); //,3435.0,9999.0,11000.0);

TempSensor* benchTempSensors[] = { &benchTempSensor, NULL };
Fan* benchFans[] = { &exhaustFan, NULL };

TempSensor* chargeCtrlsTempSensors[] = { &tempSensor1, &tempSensor2, NULL };
Fan* chargeCtrlsFans[] = { &oscillatingFan, NULL };

Device bench("Bench", benchFans, benchTempSensors );
Device controllers("Controllers", chargeCtrlsFans, chargeCtrlsTempSensors );
Device* devices[] = { &bench, &controllers, NULL };

// VCC calibrated for Samsung Chronos laptop and USB hub (with display off)
Voltmeter batteryBankVoltmeter(3, 1010000.0, 100500.0, 4.77);
Shunt batteryBankShunt;


PowerMeter batteryBankPower("Main 12V Battery Bank",&batteryBankVoltmeter,&batteryBankShunt);
PowerMeter* powerMeters[] = { &batteryBankPower, NULL };

/////////////////////////////////////////////////////////////////////////
//
// Arduino Setup
//
/////////////////////////////////////////////////////////////////////////

void setup() {
  
  Serial.begin(57600);
  
  char version[VERSION_SIZE];
  EEPROM.get(0,version);

  #ifdef VERBOSE
  Serial.print(F("#  Loaded version:"));
  Serial.println(version);
  #endif

  int powerMetersAddr = PowerMeterConfig::computeFirstElementAddr(devices);

  if ( strcmp(VERSION,version) )
  {
    gLastInfoMsg = F("Version changed.  Clearing EEPROM and saving defaults.  Old: ");
    gLastInfoMsg += version;
    gLastInfoMsg += " New: ";
    gLastInfoMsg += VERSION;
    EEPROM.put(0,VERSION);
    EEPROM.put(sizeof(VERSION), fanMode);
    for( int i = 0; devices[i] != NULL; i++ ){
      DeviceConfig config(i,devices[i]);
      config.save();
    }
    for( int i = 0; powerMeters[i] != NULL; i++ ){
      PowerMeterConfig config(powerMetersAddr,i,powerMeters[i]);
      config.save();
    }    
  }
  else
  {
    gLastInfoMsg = F("Loading EEPROM data for version ");
    gLastInfoMsg += version;
    
    #ifdef VERBOSE
    Serial.println(F("#  Loading EEPROM data."));
    #endif
    EEPROM.get(sizeof(VERSION), fanMode);
    for( int i = 0; devices[i] != NULL; i++ )
    {
      DeviceConfig config(i,devices[i]);
      config.load();
    }
    for( int i = 0; powerMeters[i] != NULL; i++ )
    {
      PowerMeterConfig config(powerMetersAddr,i,powerMeters[i]);
      config.load();
    }
  }

  for( int i = 0; powerMeters[i] != NULL; i++ )
  {
    powerMeters[i]->setup();
  }

  for( int i = 0; devices[i] != NULL; i++ )
  {
    devices[i]->setup();
  }
  
  delay(1000);
}


/////////////////////////////////////////////////////////////////////////
//
// Arduino Main Loop
//
/////////////////////////////////////////////////////////////////////////

void loop() 
{
  static unsigned int loopCnt = 0;

  bool bSerialAvailable = Serial.available();
  
  if ( bSerialAvailable || loopCnt++ % 10 == 0 )
  {
    for( int i = 0; devices[i] != NULL; i++ )
    {
      devices[i]->update();
      loopCnt = 1;    
    }
  }
  
  if ( bSerialAvailable )
  {
    char commandBuff[256];   
    memset(commandBuff,0,256);
    int bytesRead = Serial.readBytesUntil('\n', commandBuff, 256);
    
    if ( bytesRead > 0 ) {

      #ifdef VERBOSE
      Serial.print(F("#  Received: ")); Serial.print(commandBuff); Serial.println("'");
      #endif
      
      char *pszCmd = strtok(commandBuff, ", \r\n");

      int respCode = 0;
      String respMsg = "OK";

      Serial.println(F("#BEGIN#"));
      Serial.println( "{" );
      
      if ( !strcmp_P(pszCmd,PSTR("GET")) )
      {       
        Fan::printFanMode(fanMode);                           
        Serial.println(",");

        PowerMeter::printPowerMeters(powerMeters);
        Serial.println(",");
        
        Device::printDevices(devices);
        Serial.println(",");
      } 
      else if ( !strcmp_P(pszCmd,PSTR("SET_FAN_MODE")) )
      {
        const char* pszFanMode = strtok(NULL,", ");
        if ( !strcmp("ON",pszFanMode) ) {
          fanMode = FAN_MODE_ON;
          respMsg = F("OK - fanMode=ON");
        } else if ( !strcmp("OFF",pszFanMode) ) {
          fanMode = FAN_MODE_OFF;
          respMsg = F("OK - fanMode=OFF");
        } else if ( !strcmp("AUTO",pszFanMode) ) {
          fanMode = FAN_MODE_AUTO;
          respMsg = F("OK - fanMode=AUTO");
        } else {
          respCode = 102;
          respMsg = F("Expected ON|OFF|AUTO for fan mode. Received: ");
          respMsg += pszFanMode;
        }

        if ( respCode == 0 )
        {
          const char* pszSaveMode = strtok(NULL,", ");
          
          if ( !strcmp_P(pszSaveMode,PSTR("PERSIST")) )
          {
            EEPROM.put( sizeof(VERSION), fanMode);
            respMsg += " saved to EEPROM";
          } 
          else if ( !strcmp_P(pszSaveMode,PSTR("TRANSIENT")) )
          {
            // No action for transient
          } 
          else 
          {
            respCode = 103;
            respMsg = F("Expected PERSIST|TRANSIENT but found: ");
            respMsg += pszSaveMode;
          }
        }
      } 
      else if ( !strcmp_P(pszCmd,PSTR("SET_FAN_THRESHOLDS")) )
      {
        //example: SET_FAN_THRESHOLDS,*,Osc,98.0,78.0,PERSIST
        
        const char* pszDeviceFilter = strtok(NULL,",");
        const char* pszFanFilter = strtok(NULL,",");

        //TODO - no way in arduino to validate float parsing since sscanf fails on floats :-( 
        float onTemp = atof(strtok(NULL,", "));
        float offTemp = atof(strtok(NULL,", "));

        if ( onTemp <= offTemp ) 
        {
            respMsg = F("Fan ON temperature must be greater than OFF temp.");            
            respCode = 201;          
        }
        else
        {
          String updatedFans = "";
          
          for( int i = 0; devices[i] != NULL; i++ )
          {
            if ( !strcmp(pszDeviceFilter,"*") || strstr(devices[i]->name,pszDeviceFilter) )
            {
              for( int j = 0; devices[i]->fans[j] != NULL; j++ )
              {
                Fan* pFan = devices[i]->fans[j];
                if ( !strcmp(pszFanFilter,"*") || strstr(pFan->name,pszFanFilter) )
                {
                  pFan->onTemp = onTemp;
                  pFan->offTemp = offTemp;
                  if ( updatedFans.length() )
                  {
                    updatedFans += ",";
                  }
                  updatedFans += pFan->name;
                }
              }
            }
          }
  
          if (updatedFans.length())
          {
            respMsg += F(" Set fan thresholds for: ");
            respMsg += updatedFans;
            const char* pszSaveMode = strtok(NULL,", ");
            
            if ( !strcmp_P(pszSaveMode,PSTR("PERSIST")) )
            {
              for( int i = 0; devices[i] != NULL; i++ )
              {
                DeviceConfig deviceConfig(i,devices[i]);
                deviceConfig.save();
              }
            } 
            else if ( !strcmp_P(pszSaveMode,PSTR("TRANSIENT")) )
            {
              // No action for transient
            }
            else
            {
              respMsg = F("Expected PERSIST|TRANSIENT but found: ");
              respMsg += pszSaveMode;
              respCode = 203;
            }
          } 
          else
          {
            respMsg = F("No fans matched device and fan name filters");
            respCode = 204;
          }
        }
      }
      else if ( !strcmp_P(pszCmd,PSTR("SET_POWER_METER_VCC")) )
      {
        //example: SET_POWER_METER_VCC,*,4.88,PERSIST

        const char* pszMeterNameFilter = strtok(NULL,",");
        
        float vcc = atof(strtok(NULL,", "));

        String updatedMeters = "";
        
        for( int i = 0; powerMeters[i] != NULL; i++ )
        {
          if ( !strcmp(pszMeterNameFilter,"*") || strstr(powerMeters[i]->name,pszMeterNameFilter) )
          {
            powerMeters[i]->voltmeter->vcc = vcc;
            if ( updatedMeters.length() )
            {
              updatedMeters += ",";
            }
            updatedMeters += powerMeters[i]->name;

          }
        }
  
        if (updatedMeters.length())
        {
          respMsg += F(" Set vcc for: ");
          respMsg += updatedMeters;
          const char* pszSaveMode = strtok(NULL,", ");
          
          if ( !strcmp_P(pszSaveMode,PSTR("PERSIST")) )
          {
            size_t baseAddr = PowerMeterConfig::computeFirstElementAddr(devices);
            
            for( int i = 0; powerMeters[i] != NULL; i++ )
            {
              PowerMeterConfig config(baseAddr,i,powerMeters[i]);
              config.save();
            }
          } 
          else if ( !strcmp_P(pszSaveMode,PSTR("TRANSIENT")) )
          {
            // No action for transient
          }
          else
          {
            respMsg = F("Expected PERSIST|TRANSIENT but found: ");
            respMsg += pszSaveMode;
            respCode = 303;
          }
        } 
        else
        {
          respMsg = F("No power meter name matched filter: ");
          respMsg += pszMeterNameFilter;
          respCode = 304;
        }
      }      
      else
      {
        respMsg = F("Expected GET|SET_FAN_MODE|SET_FAN_THRESHOLDS|SET_POWER_METER_VCC: ");
        respMsg += pszCmd;
        respCode = -1;
      }
      
      Serial.print("  ");
      if ( gLastErrorMsg.length() ) 
      {
        if ( respCode == 0 ) 
        {
          respCode = -1;
          respMsg = gLastErrorMsg;
        }
        else
        {
          respMsg += " | ";
          respMsg += gLastErrorMsg;
        }
      }
      else if ( gLastInfoMsg.length() )
      {
        respMsg += " | ";
        respMsg += gLastInfoMsg;
      }
      JPRINT_NUMBER(respCode);
      Serial.println(",");
      Serial.print("  ");
      JPRINT_STRING(respMsg);
      Serial.println();
      Serial.println("}");

      Serial.println(F("#END#"));

      // clear last error AFTER response sent to save memory
      gLastErrorMsg = "";    
      gLastInfoMsg = "";
    }
  } 
  delay(500);

 }

