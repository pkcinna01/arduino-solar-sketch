//
// Change version if code changes impact data structures in EEPROM.
// A version change will reset EEPROM data to defaults.
//
#define VERSION "SOLAR-1.8"
#define BUILD_NUMBER 7
#define BUILD_DATE __DATE__

#include <EEPROM.h>
#include <Wire.h>
#include <ArduinoSTL.h>

// automation includes not specific to arduino
#include "automation/Automation.h"
#include "automation/Sensor.h"
#include "automation/CompositeSensor.h"
#include "automation/constraint/Constraint.h"
#include "automation/constraint/NotConstraint.h"
#include "automation/constraint/ValueConstraint.h"
#include "automation/constraint/TimeRangeConstraint.h"
#include "automation/constraint/AndConstraint.h"
#include "automation/Sensor.cpp"

using namespace automation;

String gLastErrorMsg;
String gLastInfoMsg;

#include "arduino/Arduino.h"
#include "arduino/JsonWriter.h"
#include "arduino/Dht.h"
#include "arduino/DhtHumiditySensor.h"
#include "arduino/DhtTempSensor.h"
#include "arduino/ThermistorSensor.h"
#include "arduino/LightSensor.h"
#include "arduino/VoltageSensor.h"
#include "arduino/CurrentSensor.h"
#include "arduino/PowerSensor.h"
#include "arduino/CoolingFan.h"
#include "arduino/PowerSwitch.h"
#include "arduino/Automation.cpp"
#include "arduino/Sensor.cpp"
#include "arduino/Device.cpp"
#include "arduino/Capability.cpp"

#include <vector>
#include <sstream>
#include <algorithm>
#include <string.h>

using namespace std;
using namespace arduino;

ThermistorSensor charger1Temp("Charger 1 Temp",0),
                 charger2Temp("Charger 2 Temp",1),
                 sunroomTemp("Sunroom Temp",2),
                 atticTemp("Attic Temp",3),
                 inverterTemp("Inverter Temp",4);

LightSensor lightLevel("Sunshine Level",5);                 
                
Dht dht(5);
DhtTempSensor enclosureTemp("Enclosure Temp",dht);
DhtHumiditySensor enclosureHumidity("Enclosure Humidity",dht);
VoltageSensor batteryBankVoltage("Battery Bank Voltage", 9, 1016000,101100);
//VoltageSensor batteryBankVoltage2("Battery Bank Voltage2", 10, 1016000,101100);

CurrentSensor batteryBankCurrent("Battery Bank Current");
PowerSensor batteryBankPower("Battery Bank Power", &batteryBankVoltage, &batteryBankCurrent);
vector<Sensor*> chargerGrpSensors { &charger1Temp, &charger2Temp };
CompositeSensor chargerGroupTemp("Charger Group Temp", chargerGrpSensors, Sensor::maximum);

//TimeRangeConstraint chargerGroupFanEnabledTimeRange( { 8, 0, 0 }, { 18, 30, 0 } );

arduino::CoolingFan exhaustFan("Enclosure Exhaust Fan", 22, enclosureTemp, 90, 89,LOW);
arduino::CoolingFan chargerGroupFan("Charger Group Fan", 23, chargerGroupTemp, 90, 89,LOW);
arduino::CoolingFan inverterFan("Inverter Fan", 24, inverterTemp, 90, 89,LOW);

struct MinBatteryBankVoltage : MinConstraint<float,Sensor&> {
  MinBatteryBankVoltage(float volts) : MinConstraint(volts, batteryBankVoltage) {    
  }
};

struct MinBatteryBankVoltage minBatteryBankVoltage(21);
struct MinBatteryBankVoltage outletsMinBatteryBankVoltage(22);

struct MaxBatteryBankPower : MaxConstraint<float,Sensor&> {
  MaxBatteryBankPower(float watts) : MaxConstraint(watts, batteryBankPower) {    
  }
};

struct InverterSwitch : public arduino::PowerSwitch {
  InverterSwitch() : arduino::PowerSwitch("Inverter Switch", 25, LOW) {
    //setConstraint(&minBatteryBankVoltage);
  }
} inverterSwitch;

struct OutletSwitch : public arduino::PowerSwitch {
  struct MaxBatteryBankPower maxBatteryBankPower {1400};
  AndConstraint constraints { {&outletsMinBatteryBankVoltage, &maxBatteryBankPower} };
  OutletSwitch(const string& name, int pin, int onValue = HIGH) : arduino::PowerSwitch(name, pin, onValue) {
    setConstraint(&constraints);
  }
};

struct Outlet1Switch : public OutletSwitch {
  Outlet1Switch() : OutletSwitch("Outlet 1 Switch", 26) {}
} outlet1Switch;

struct Outlet2Switch : public OutletSwitch {
  Outlet2Switch() : OutletSwitch("Outlet 1 Switch", 27) {}
} outlet2Switch;


Device* devices[] = { 
    &exhaustFan, &chargerGroupFan, &inverterFan, &inverterSwitch, nullptr
};

Sensor* sensors[] = { 
    &chargerGroupTemp, 
    &batteryBankVoltage, &batteryBankCurrent, &batteryBankPower,
    &charger1Temp, &charger2Temp, &inverterTemp, &sunroomTemp, &atticTemp, 
    &enclosureTemp, &enclosureHumidity, 
    &exhaustFan.toggleSensor, &chargerGroupFan.toggleSensor, 
    &inverterFan.toggleSensor, &inverterSwitch.toggleSensor,
    &outlet1Switch.toggleSensor, &outlet2Switch.toggleSensor,
    &lightLevel, nullptr
};

void setup() {
  //Serial.begin(38400, SERIAL_8O1); // bit usage: 8 data, odd parity, 1 stop
  Serial.begin(57600);

  char version[VERSION_SIZE];
  EEPROM.get(0,version);
  if ( strcmp(VERSION,version) )
  {
    gLastInfoMsg = F("Version changed. Clearing EEPROM and saving defaults.  Old: ");
    gLastInfoMsg += version;
    gLastInfoMsg += " New: ";
    gLastInfoMsg += VERSION;
    EEPROM.put(0,VERSION);
    EEPROM.put(sizeof(VERSION), JsonSerialWriter::format);
    /*for( int i = 0; devices[i] != NULL; i++ ){
      DeviceConfig config(i,devices[i]);
      config.save();
    }
    for( int i = 0; powerMeters[i] != NULL; i++ ){
      PowerMeterConfig config(powerMetersAddr,i,powerMeters[i]);
      config.save();
    } */   
  }
  else
  {
    gLastInfoMsg = F("Loading EEPROM data for version ");
    gLastInfoMsg += version;
    
    EEPROM.get(sizeof(VERSION), JsonSerialWriter::format);
    /*
    for( int i = 0; devices[i] != NULL; i++ )
    {
      DeviceConfig config(i,devices[i]);
      config.load();
    }
    for( int i = 0; powerMeters[i] != NULL; i++ )
    {
      PowerMeterConfig config(powerMetersAddr,i,powerMeters[i]);
      config.load();
    }*/
  }
  for(int i=0; sensors[i] != nullptr; i++) {
    sensors[i]->setup();
  }
  for(int i=0; devices[i] != nullptr; i++) {
    devices[i]->setup();
  }
}

bool findAndSetCapability(vector<Capability*>& capabilities, const char* pszCapabilityType, const char* pszTargetState, ostream& respMsgStream) {
  float targetValue = !strcasecmp(pszTargetState,"on") ? 1 : !strcasecmp(pszTargetState,"off") ? 0 : atof(pszTargetState);
  bool bUpdated = false;
  for ( auto cap : capabilities ) {
    if ( cap->getType() == pszCapabilityType ) {
      cap->setValue(targetValue);
      if ( !bUpdated ) {
        respMsgStream << ",";
        bUpdated = true;
      }
      respMsgStream << cap->getTitle() << ":" << cap->getValue();
    }
  }
  return bUpdated;
}

void loop() {
  static unsigned long lastUpdateTimeMs = 0, beginCmdReadTimeMs = 0;
  static unsigned int updateIntervalMs = 10000;
  static char commandBuff[200];
  static size_t bytesRead = 0; 

  bool msgSizeExceeded = false;
  bool cmdReady = false;
  bool msgReadTimedOut = false;
  unsigned long currentTimeMs = millis();

  // read char by char to avoid issues with default 64 byte serial buffer
  while (Serial.available() ) {        
    char c = Serial.read();
    if ( bytesRead == 0 ) {
      if ( c == -1 )
        continue;
      else 
        beginCmdReadTimeMs = millis();
    }
    commandBuff[bytesRead++] = c;
    if (c == '\n') {
      commandBuff[bytesRead] = '\0';
      cmdReady = true;
      break;
    } else if ( bytesRead >= sizeof commandBuff ) {
      commandBuff[bytesRead-1] = '\0';
      msgSizeExceeded = true;
      break;
    } 
  }

if ( cmdReady || (currentTimeMs - lastUpdateTimeMs) > updateIntervalMs )
  {
    for(int i=0; devices[i] != nullptr; i++) {
      Device* pDevice = devices[i];
      pDevice->applyConstraint(false);
    } 
    lastUpdateTimeMs = currentTimeMs;
  } else if ( beginCmdReadTimeMs > 0 && (currentTimeMs - beginCmdReadTimeMs) > updateIntervalMs ) {
    msgReadTimedOut = true;
  }
  
  if ( cmdReady || msgReadTimedOut ) {

    char *pszCmd = strtok(commandBuff, "|");            
    char *pszRequestId = strtok(NULL,"|");
    unsigned int requestId = atoi(pszRequestId);
    
    int respCode = 0;
    String respMsg = "OK";
    const __FlashStringHelper * respMsgKey = F("respMsg");
    
    JsonSerialWriter writer;
    JsonSerialWriter::clearByteCount();
    JsonSerialWriter::clearChecksum();
    writer.implPrint(F("#BEGIN:"));
    writer.implPrint(requestId);
    writer.implPrintln("#");
    writer.println( "{" );
    writer.increaseDepth();
    if ( msgReadTimedOut ) 
    {
      writer.beginStringObj(respMsgKey);
      //writer + F("Serial data read timed out.  Bytes received: ") + bytesRead;
      respCode = 101;
    }
    else if ( msgSizeExceeded )
    {
      writer.beginStringObj(respMsgKey);
      //writer + F("Request exceeded maximum size. Bytes read: ") + bytesRead;
      respCode = 102;
    }
    else
    {
      char *pszCmdName = strtok(pszCmd, ", \r\n");
      if ( !strcmp_P(pszCmdName,PSTR("SET")) )
      {
        writer.beginStringObj(respMsgKey);
        const char* pszArg = strtok(NULL,", \r\n");
        if ( !strcmp_P(pszArg,PSTR("OUTPUT_FORMAT")) ) 
        {
          const char* pszFormat = strtok(NULL,", \r\n");
          const char* pszPersistMode = strtok(NULL,", \r\n");
          respCode = writer.processSetFormatCommand(pszFormat,pszPersistMode);
        }
        else if ( !strcmp_P(pszArg,PSTR("TIME_T")) ) 
        {
          const char* pszDateTime = strtok(NULL,", ");
          time_t time = atol(pszDateTime);
          setTime(time);
          writer + F("TIME_T set to ") + time;
        }
        else if ( !strcmp_P(pszArg,PSTR("TIME")) ) 
        {
          int year = atoi(strtok(NULL,", ")), month = atoi(strtok(NULL,", ")), day =atoi(strtok(NULL,", ")),
              hour =atoi(strtok(NULL,", ")), minute = atoi(strtok(NULL,", ")), second = atoi(strtok(NULL,", \r\n")); 
          setTime(hour,minute,second,day,month,year);
          writer + F("TIME set using YYYY,MM,DD,HH,mm,SS args");
        }
        else if ( !strcmp_P(pszArg,PSTR("DEVICE_CAPABILITY")) ) 
        {
          const char* pszName = strtok(NULL,",\r\n");
          const char* pszCapabilityType = strtok(NULL,", \r\n");
          stringstream ss;
          bool bUpdated = false;
          const char* pszTargetState = strtok(NULL,", \r\n");
          for( Device* pDevice : devices ) {
            if ( pDevice->name == pszName || !strcmp_P(pszName,PSTR("*")) ) {
              bUpdated |= findAndSetCapability(pDevice->capabilities, pszCapabilityType, pszTargetState, ss);
            }
          }
          if ( bUpdated ) {
             writer + ss.str().c_str();
             
             // do persist?
          }
        }
        else if ( !strcmp_P(pszArg,PSTR("DEVICE_MODE")) ) 
        {
          const char* pszName = strtok(NULL,",\r\n");
          const char* pszMode = strtok(NULL,", \r\n");
          stringstream ss;
          bool bUpdated = false;
          for(int i=0; devices[i] != nullptr; i++) {
            Device* pDevice = devices[i];
            if ( !pDevice->pConstraint ) 
              continue;
            if ( !strcmp(pDevice->name.c_str(),pszName) || !strcmp_P(pszName,PSTR("*")) ) {
              Constraint::Mode mode = Constraint::parseMode(pszMode);  
              pDevice->pConstraint->mode = mode;
              pDevice->applyConstraint();
              if ( bUpdated ) {
                ss << ", ";
              } else {
                bUpdated = true;
              }
              ss << pDevice->name << " mode:" << Constraint::modeToString(pDevice->pConstraint->mode);
            }
          }
          if ( bUpdated ) {
            writer + ss.str().c_str();
            // do persist?
          } else {
            writer + F("No device mode updates.  Device name filter: ");
            writer + pszName;
          }
        }
        else
        {
          writer + F("Expected OUTPUT_FORMAT|TIME_T|TIME|DEVICE_MODE|DEVICE_CAPABILITY but found: ") + pszArg;
          respCode = 203;
        }       
      }
      else if ( !strcmp_P(pszCmdName,PSTR("GET")) )
      {  
        const char* pszArg;
        while ( (pszArg=strtok(NULL,", \r\n")) != nullptr ) {
          if ( !strcmp_P(pszArg,PSTR("ENV")) )
          {
            writer.printKey("env");
            writer.println("{");
            writer.increaseDepth();
            writer.printlnStringObj(F("release"),VERSION,",")
            .printlnNumberObj(F("buildNumber"),BUILD_NUMBER,",")
            .printlnStringObj(F("buildDate"),BUILD_DATE,",")
            .printlnStringObj(F("Vcc"),readVcc(),",")
            .beginStringObj("time");
            time_t t = now();
            writer + year(t) + "-" + month(t) + "-" + day(t) + " " + hour(t) + ":" + minute(t) + ":" + second(t);
            writer.endStringObj();
            writer.noPrefixPrintln(",");
            writer.printlnStringObj("timeSet", timeStatus() == timeSet ? "YES" : "NO");
            writer.decreaseDepth();
            writer.println("},");
          } 
          else if ( !strcmp_P(pszArg,PSTR("OUTPUT_FORMAT")) ) 
          {
            writer.printlnStringObj(F("outputFormat"), JsonSerialWriter::formatAsString(),",");
          }
          else if ( !strcmp_P(pszArg,PSTR("SENSORS")) ) 
          {
            writer.printlnArrayObj(F("sensors"),sensors,",");
          }
          else if ( !strcmp_P(pszArg,PSTR("DEVICES")) ) 
          {
            writer.printlnArrayObj(F("devices"),devices,",");
          }
          else if ( !strcmp_P(pszArg,PSTR("TIME")) ) 
          {
            time_t t = now();
            writer.printKey("time");
            writer.println("{");
            writer.increaseDepth();
            writer.printlnNumberObj("year", year(t), ",");
            writer.printlnNumberObj("month", month(t), ",");
            writer.printlnNumberObj("day", day(t), ",");
            writer.printlnNumberObj("hour", hour(t), ",");
            writer.printlnNumberObj("minute", minute(t), ",");
            writer.printlnNumberObj("second", second(t), ",");
            writer.printlnStringObj("timeSet", timeStatus() == timeSet ? "YES" : "NO", ",");
            writer.decreaseDepth();
            writer.println("},");
          }
          else 
          {
            writer.beginStringObj(respMsgKey);
            writer + F("GET command expected argument {SENSORS|DEVICES|OUTPUT_FORMAT|TIME|ENV} but found: '") + pszArg + "'. ";
            respCode = 401;
          }
        }
        if ( !respCode ) {
          writer.beginStringObj(respMsgKey);
        }
      }    
      else
      {
        writer.beginStringObj(respMsgKey)
          + F("Expected GET|SET but found: ") + pszCmdName;
        respCode = 601;
      }
    }
    
    if ( gLastErrorMsg.length() ) 
    {        
      if ( respCode == 0 ) 
      {
        respCode = -1;
      }
      if ( writer.getOpenStringValByteCnt() > 0 ) 
        writer + " | ";
      writer + gLastErrorMsg;
    }
    else if ( gLastInfoMsg.length() )
    {
      if ( writer.getOpenStringValByteCnt() > 0 ) 
        writer + " | ";
      writer + gLastInfoMsg;
    }
    if ( writer.getOpenStringValByteCnt() <= 0 ) 
    {
      if ( respCode == 0 ) 
      {
        writer + F("OK");
      }
      else
      {
        writer + F("ERROR");
      }
    }
    writer.endStringObj(",");
    writer.noPrefixPrintln("");
    writer.printlnNumberObj(F("respCode"),respCode);
    writer.decreaseDepth();
    writer.print("}");
    writer.implPrint(F("\n#END:"));
    writer.implPrint(requestId);
    writer.implPrint(":");
    writer.implPrint(JsonSerialWriter::getByteCount());
    writer.implPrint(":");
    writer.implPrint(JsonSerialWriter::getChecksum());
    writer.implPrintln("#");

    // clear last error AFTER response sent to save memory
    gLastErrorMsg = "";    
    gLastInfoMsg = "";

    bytesRead = 0; // reset commandBuff
    beginCmdReadTimeMs = 0;
  }    
}
