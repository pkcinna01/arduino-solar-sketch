//////////////////////////////////////////////////////////////////////////
//
// Project enables/disables fans based on temperature monitoring devices
// 
// A device can have multiple fans and multiple temperature guages.  All 
// fans for a device are turned on/off based on the max temperature probe
// reading of that device.
//
// This current setup is controlling 2 fans
// 1) exhaust fan on enclosure wall controlled with a single DHT temp sensor
// 2) An oscillating fan cooling 2 epsolar charge controllers where each has 
//    a dedicated thermistor temp sensor. 
//
// The serial bus support allows info about fans and temperatures to be
// monitored with Prometheus and displayed in Grafana.  The EPSolar
// charge controller metrics from monitoring can override the arduino fan
// regulation, A Grafana or Prometheus alert will trigger a 
// "SET_FAN_MODE,0N,TRANSIENT" command to temporarily take control.
// "SET_FAN_MODE,AUTO,TRANSIENT" will give control back to arduino temp sensors.
// Prometheus gets arduino metrics using the Java "arduino-client" in same
// parent folder as this project.
//
//////////////////////////////////////////////////////////////////////////

#include <DHT.h>
#include <EEPROM.h>
#include <string.h>

using namespace std;

#define VERSION_SIZE 14
#define FAHRENHEIT true

//
// Change this version if code changes impact data structures in EEPROM.
// Warning: Version change will reset EEPROM data to defaults
//
#define VERSION "SOLAR-1.0.0.0"

typedef unsigned char FanMode;

const FanMode FAN_MODE_OFF = 0, FAN_MODE_ON = 0x1, FAN_MODE_AUTO = 0x2;

FanMode fanMode = FAN_MODE_AUTO;

//#define DEBUG
//#define VERBOSE

#define JSTRING(s) String("\"") + #s + "\": \"" + s + "\""
#define JNUMBER(n) String("\"") + #n + "\": " + n
#define JBOOLEAN(b) String("\"") + #b + "\": " + (b?"true":"false")
#define JKEY(k) String("\"") + #k + "\": "

#define JPTRARRAY(arr) Serial.print(String("\"") + #arr + "\": [\n    ");\
  for( int i = 0; arr[i] != NULL; i++ ){\
    if ( i != 0 )\
      Serial.print(",\n    ");\
    arr[i]->print();\
  };\
  Serial.print("\n    ]");

class Fan {
  public:
  String name;
  float onTemp;
  float offTemp;
  int relayPin;
  bool onValue = HIGH; // some relays are on when signal is set low/false instead of high/true

  static void printFanMode(FanMode mode) {
    String fanModeText = fanMode == FAN_MODE_AUTO ? "AUTO" 
                   : fanMode == FAN_MODE_ON ? "ON" 
                   : fanMode == FAN_MODE_OFF ? "OFF" 
                   : "INVALID";
    Serial.print("  "); 
    Serial.print( JNUMBER(fanMode) + ", " );
    Serial.print( JSTRING(fanModeText) );
  }
  
  Fan(String name, int relayPin, float onTemp = 125, float offTemp = 115, bool onValue = HIGH) :
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
    Serial.print( "#'" ); Serial.print(name); Serial.print("' fan (relayPin="); 
    Serial.print(relayPin); Serial.println(") mode set to OUTPUT");
    #endif
  }

  virtual void update(float temp)
  {
    bool bTurnOn = false;
    bool bTurnOff = false;

    switch ( fanMode ) {
      case FAN_MODE_ON: bTurnOn = true; break;
      case FAN_MODE_OFF: bTurnOff = true; break;
      case FAN_MODE_AUTO: 
        bTurnOn = temp >= onTemp;
        bTurnOff = temp < offTemp;
    }

    int relayPinVal = digitalRead(relayPin);
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
    Serial.print(JSTRING(name));
    Serial.print(", ");
    Serial.print(JNUMBER(relayPin));
    Serial.print(", ");
    Serial.print(JNUMBER(onTemp));
    Serial.print(", ");
    Serial.print(JNUMBER(offTemp));
    Serial.print(", ");
    int relayValue = digitalRead(relayPin);
    Serial.print(JNUMBER(relayValue));
    Serial.print(", ");
    bool on = relayValue == onValue;
    Serial.print(JBOOLEAN(on));
    Serial.print(" }");
  }
};



class TempSensor {
  public:
  String name;
  int sensorPin;

  float beta; //3950.0,  3435.0 
  float balanceResistance, roomTempResistance;
  
  typedef float(TempSensor::*SampleMethod)(void);
  
  TempSensor(String _name,
             int _sensorPin, 
             float _beta = 3950, 
             float _balanceResistance = 9999.0, 
             float _roomTempResistance = 10000.0):
    name(_name),
    sensorPin(_sensorPin),
    beta(_beta),
    balanceResistance(_balanceResistance),
    roomTempResistance(_roomTempResistance)
  {
  }

  virtual void setup()
  {
    pinMode(sensorPin,INPUT);
    #ifdef DEBUG
    Serial.print("#  '"); Serial.print(name); Serial.print("' thermistor analog input pin:"); Serial.println(sensorPin);
    #endif
  }
  
  virtual float readTemp() {  
    return readSampled(&TempSensor::readBetaCalculatedTemp);
  }

  float readSampled(SampleMethod sampleMethod){
    const unsigned char SAMPLE_NUMBER = 10;
    float adcSamplesSum = 0;
    for (int i = 0; i < SAMPLE_NUMBER; i++) {
      adcSamplesSum += (this->*sampleMethod)();
      delay(25);
    }
    return adcSamplesSum/SAMPLE_NUMBER;
  }

  float readBetaCalculatedTemp() {
    const float ROOM_TEMP = 298.15;

    int pinVoltage = analogRead(sensorPin);
    float rThermistor = balanceResistance * ( (1023.0 / pinVoltage) - 1);
    float tKelvin = (beta * ROOM_TEMP) / 
            (beta + (ROOM_TEMP * log(rThermistor / roomTempResistance)));

    float tCelsius = tKelvin - 273.15;
    float tFahrenheit = (tCelsius * 9.0)/ 5.0 + 32.0;
    return tFahrenheit;
  }
  
  virtual void print()
  {
    float temp = readTemp();
    Serial.print("  { ");
    Serial.print(JSTRING(name));
    Serial.print(", ");
    Serial.print(JNUMBER(temp));
    Serial.print(" }");
  }
};



class DhtTempSensor : public TempSensor {

  public:
  int dhtType;
  DHT dht;
  
  DhtTempSensor(String _name, int _sensorPin, int _dhtType = DHT22) : 
    TempSensor(_name,_sensorPin),
    dhtType(_dhtType),
    dht(_sensorPin,_dhtType)
  {    
  }

  virtual void setup()
  {
    dht.begin();
    #ifdef DEBUG
    Serial.print("#DHT"); Serial.print(dhtType); Serial.print(" started on digital pin "); Serial.println(sensorPin);
    #endif
  }  

  virtual float readHumidity()
  {
    return dht.readHumidity();  // result may be up to 2 seconds old and take 250 ms to read
  }
  
  virtual float readTemp()
  {
    return dht.readTemperature(FAHRENHEIT);
  }

  virtual void print()
  {
    float temp = readTemp();
    float humidity = readHumidity();
    //float heatIndex = dht.computeHeatIndex(temp, humidity, FAHRENHEIT);
    Serial.print("  { ");
    Serial.print(JSTRING(name));
    Serial.print(", ");
    Serial.print(JNUMBER(temp));
    Serial.print(", ");
    Serial.print(JNUMBER(humidity));
    //Serial.print(", ");
    //Serial.print(JNUMBER(heatIndex));
    Serial.print(" }");
  }
};


class Device {
  public:
  String name;
  Fan** fans;
  TempSensor** tempSensors;

  static void printDevices(Device** devices) {
    Serial.print( "  " + JKEY(devices) );
    Serial.print("[");
    for( int i = 0; devices[i] != NULL; i++ ){
      if ( i != 0 ) {
        Serial.print(",");
      }
      Serial.println();
      devices[i]->print();    
    }
    Serial.print("]");
  }
  
  Device(String _name, Fan** _fans, TempSensor** _tempSensors) :
    name(_name),
    fans(_fans),
    tempSensors(_tempSensors)
  {
  }

  virtual void setup()
  {
    #ifdef DEBUG
    Serial.print("#  "); Serial.print(name); Serial.println(" setup() begin...");
    #endif
    
    for( int i = 0; tempSensors[i] != NULL; i++ ) {
      tempSensors[i]->setup();
    }
    for( int i = 0; fans[i] != NULL; i++ ) {
      fans[i]->setup();
    }
    
    #ifdef DEBUG
    Serial.print("#  "); Serial.print(name); Serial.println(" setup() complete.");
    #endif
  }

  virtual void update() 
  {
    float maxTemp = 0;
    for( int i = 0; tempSensors[i] != NULL; i++ ) {
      float temp = tempSensors[i]->readTemp();
      if ( temp > maxTemp ) {
        maxTemp = temp;
      }
    }
    // enable/disable all fans for this device based on highest temp sensor value
    for( int i = 0; fans[i] != NULL; i++ ) {
      fans[i]->update(maxTemp);
    }
  }

  virtual void print()
  {
    Serial.print("  { ");
    Serial.print(JSTRING(name));
    Serial.print(",\n    ");
    JPTRARRAY(tempSensors);
    Serial.print(",\n    ");
    JPTRARRAY(fans);
    Serial.println();
    Serial.print("  ");
    Serial.print("}");
  }
};

class DeviceConfig {
  public:
  int index;
  Device* pDevice;

  // persistant fields (up 5 fans)
  float fanOnTemps[5];
  float fanOffTemps[5];

  DeviceConfig(int index, Device* pDevice) :
    index(index),
    pDevice(pDevice)
  {
  }

  void load() {
    int addr = sizeof(VERSION) + sizeof(fanMode) + index * (sizeof(fanOnTemps)+sizeof(fanOffTemps));
    EEPROM.get( addr, fanOnTemps );
    EEPROM.get( addr + sizeof(fanOnTemps), fanOffTemps );
    for ( int i = 0; pDevice->fans[i] != NULL; i++ ) {
      Fan* pFan = pDevice->fans[i];
      pFan->onTemp = fanOnTemps[i];
      pFan->offTemp = fanOffTemps[i];
    }
  }

  void save() {
    int addr = sizeof(VERSION) + sizeof(fanMode) + index * (sizeof(fanOnTemps)+sizeof(fanOffTemps));
    memset(fanOnTemps, 0, sizeof(fanOnTemps));
    memset(fanOffTemps, 0, sizeof(fanOffTemps));
    for ( int i = 0; pDevice->fans[i] != NULL; i++ ) {
      Fan* pFan = pDevice->fans[i];
      fanOnTemps[i] = pFan->onTemp;
      fanOffTemps[i] = pFan->offTemp;
    }
    EEPROM.put( addr, fanOnTemps );
    EEPROM.put( addr + sizeof(fanOnTemps), fanOffTemps );
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
  Serial.print("#  Loaded version:");
  Serial.println(version);
  #endif
  
  if ( String(version) != VERSION ) {
    #ifdef VERBOSE
    Serial.println("#  EEPROM version not valid.  saving default config.");
    #endif
    EEPROM.put(0,VERSION);
    EEPROM.put(sizeof(VERSION), fanMode);
    for( int i = 0; devices[i] != NULL; i++ ){
      DeviceConfig deviceConfig(i,devices[i]);
      deviceConfig.save();
    }
  } else {
    #ifdef VERBOSE
    Serial.println("#  Loading EEPROM data.");
    #endif
    EEPROM.get(sizeof(VERSION), fanMode);
    for( int i = 0; devices[i] != NULL; i++ ){
      DeviceConfig deviceConfig(i,devices[i]);
      deviceConfig.load();
    }
  }
  
  for( int i = 0; devices[i] != NULL; i++ ){
    devices[i]->setup();
  }
  
  delay(1000);
  /*
  Serial.println("{");
  Fan::printFanMode(fanMode);
  Serial.println(",");
  Device::printDevices(devices);
  Serial.println();
  Serial.println("}");
  */
}


/////////////////////////////////////////////////////////////////////////
//
// Arduino Main Loop
//
/////////////////////////////////////////////////////////////////////////

void loop() {
  
  static unsigned int loopCnt = 0;
  
  if ( loopCnt++ % 10 == 0 ) {
    for( int i = 0; devices[i] != NULL; i++ ){
      devices[i]->update();    
    }
  }
  
  if ( Serial.available() ) {
    #ifdef DEBUG
    Serial.println("#  Reading serial port...");
    #endif
    char commandBuff[256];   
    memset(commandBuff,0,256);
    int bytesRead = Serial.readBytesUntil('\n', commandBuff, 256);

    if ( bytesRead > 0 ) {

      #ifdef VERBOSE
      Serial.print("#  Received: "); Serial.print(commandBuff); Serial.println("'");
      #endif

      char *pszCmd = strtok(commandBuff, ", ");

      int respCode = 0;
      String respMsg = "OK";

      Serial.println("#BEGIN#");
      Serial.println( "{" );
  
      if ( !strcmp(pszCmd,"GET") ) {

        Fan::printFanMode(fanMode);                           
        Serial.println(",");
        Device::printDevices(devices);
        Serial.println(",");

      } else if ( !strcmp(pszCmd,"SET_FAN_MODE") ) {

        String strMode(strtok(NULL,", "));
        if ( strMode == "ON" ) {
          fanMode = FAN_MODE_ON;
          respMsg = "OK - fanMode=ON";
        } else if ( strMode == "OFF" ) {
          fanMode = FAN_MODE_OFF;
          respMsg = "OK fanMode=OFF";
        } else if ( strMode == "AUTO" ) {
          fanMode = FAN_MODE_AUTO;
          respMsg = "OK fanMode=AUTO";
        } else {
          respCode = 102;
          respMsg = "Expected ON|OFF|AUTO";          
        }

        if ( respCode == 0 ) {
          String strSave(strtok(NULL,", "));
          if ( strSave == "PERSIST" ) {
            EEPROM.put( sizeof(VERSION), fanMode);
            respMsg += " saved to EEPROM";
          } else if ( strSave == "TRANSIENT" ) {
            EEPROM.put( sizeof(VERSION), FAN_MODE_AUTO);
          } else {
            respCode = 103;
            respMsg = "Expected PERSIST|TRANSIENT: " + strSave;
          }
        }
        
      } else if ( !strcmp(pszCmd,"SET_FAN_THRESHOLDS") ) {

        //example: SET_FAN_THRESHOLDS,*,Osc,98.0,78.0,PERSIST
        
        const char* pszDeviceFilter = strtok(NULL,",");
        const char* pszFanFilter = strtok(NULL,",");

        //TODO - no way in arduino to validate float parsing since sscanf fails on floats :-( 
        float onTemp = atof(strtok(NULL,", "));
        float offTemp = atof(strtok(NULL,", "));
        String updatedFans = "";
        
        for( int i = 0; devices[i] != NULL; i++ ) {
          if ( !strcmp(pszDeviceFilter,"*") || strstr(devices[i]->name.c_str(),pszDeviceFilter) ) {
            for( int j = 0; devices[i]->fans[j] != NULL; j++ ) {
              Fan* pFan = devices[i]->fans[j];
              if ( !strcmp(pszFanFilter,"*") || strstr(pFan->name.c_str(),pszFanFilter) ) {
                pFan->onTemp = onTemp;
                pFan->offTemp = offTemp;
                if ( updatedFans.length() ) {
                  updatedFans += ",";
                }
                updatedFans += pFan->name;
              }
            }
          }
        }

        if (updatedFans.length()){
          respMsg += " Updated fans: " + updatedFans;
          String saveMode = strtok(NULL,", ");
          if ( saveMode == "PERSIST" ) {
            for( int i = 0; devices[i] != NULL; i++ ){
              DeviceConfig deviceConfig(i,devices[i]);
              deviceConfig.save();
            }
          } else if ( saveMode != "TRANSIENT" ) {
            respMsg = "Invalid persistance mode: " + saveMode;
            respCode = 202;
          }
        } else {
          respMsg = "No fans matched filters";
          respCode = 203;
        }
      } else {
          respCode = -1;
          respMsg = String("Unsupported command: '") + pszCmd + "'";
      }
  
      Serial.println("  " + JNUMBER(respCode) + ",");
      Serial.println("  " + JSTRING(respMsg));
      Serial.println("}");
      Serial.println("#END#");
    }
  } 
  delay(500);

 }




