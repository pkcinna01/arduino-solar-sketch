//////////////////////////////////////////////////////////////////////////
//
// Project enables/disables fans based on temperature monitoring devices
// and monitors power usage.
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
// The serial bus API provides fan and temperature details for Prometheus and
// Grafana.  The EPSolar charge controller metrics from monitoring can override
// the arduino fan regulation.  A Grafana or Prometheus alert can trigger a
// "SET_FAN_MODE,ON,TRANSIENT" command to temporarily take control.
// "SET_FAN_MODE,AUTO,TRANSIENT" will give control back to arduino temp sensors.
// Prometheus gets arduino metrics using the Java "solar-client" in same
// parent folder as this project.
// 
//////////////////////////////////////////////////////////////////////////

//
// Change version if code changes impact data structures in EEPROM.
// A version change will reset EEPROM data to defaults.
//
#define VERSION "SOLAR-1.8"
#define BUILD_NUMBER 7
#define BUILD_DATE __DATE__

#ifndef ARDUINO // HACK for intellisense in VSCODE
  #define ARDUINO 10805  
  #include <iom328p.h>
#endif

#include <DHT.h>
#include <EEPROM.h>
#include <string.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>

#define VERSION_SIZE 10
#define FAHRENHEIT true

String gLastErrorMsg;
String gLastInfoMsg;

#include "solar/JsonWriter.h"

#include "solar/Fan.h"
#include "solar/TempSensor.h"
#include "solar/DhtTempSensor.h"
#include "solar/Device.h"
#include "solar/Shunt.h"
#include "solar/Voltmeter.h"
#include "solar/PowerMeter.h"
#include "solar/DeviceConfig.h"
#include "solar/PowerMeterConfig.h"

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

Voltmeter batteryBankVoltmeter(3, 1010000.0, 100500.0, 4.78);
Shunt batteryBankShunt;

PowerMeter batteryBankPower("Main 12V Battery Bank",&batteryBankVoltmeter,&batteryBankShunt);
PowerMeter* powerMeters[] = { &batteryBankPower, NULL };

/////////////////////////////////////////////////////////////////////////
//
// Arduino Setup
//
/////////////////////////////////////////////////////////////////////////

void setup() {
  
  //Serial.begin(57600, SERIAL_8N1);
  Serial.begin(38400, SERIAL_8O1); // bit usage: 8 data, odd parity, 1 stop
  
  char version[VERSION_SIZE];
  EEPROM.get(0,version);

  int powerMetersAddr = PowerMeterConfig::computeFirstElementAddr(devices);

  if ( strcmp(VERSION,version) )
  {
    gLastInfoMsg = F("Version changed. Clearing EEPROM and saving defaults.  Old: ");
    gLastInfoMsg += version;
    gLastInfoMsg += " New: ";
    gLastInfoMsg += VERSION;
    EEPROM.put(0,VERSION);
    EEPROM.put(sizeof(VERSION), Fan::mode);
    EEPROM.put(sizeof(VERSION)+sizeof(Fan::mode), JsonSerialWriter::format);
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
    
    EEPROM.get(sizeof(VERSION), Fan::mode);

    EEPROM.get(sizeof(VERSION) + sizeof(Fan::mode), JsonSerialWriter::format);
    
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
    lastUpdateTimeMs = currentTimeMs;
    for( int i = 0; devices[i] != NULL; i++ )
    {
      devices[i]->update();
    }
  } else if ( beginCmdReadTimeMs > 0 && (currentTimeMs - beginCmdReadTimeMs) > updateIntervalMs ) {
    msgReadTimedOut = true;
  }
  
  if ( cmdReady || msgReadTimedOut ) {

    char *pszCmd = strtok(commandBuff, "|");            
    char *pszRequestId = strtok(NULL,"|");
    unsigned int requestId = atoi(pszRequestId);
    
    int respCode = 0;
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
      writer + F("Serial data read timed out.  Bytes received: ") + bytesRead;
      respCode = 101;
    }
    else if ( msgSizeExceeded )
    {
      writer.beginStringObj(respMsgKey);
      writer + F("Request exceeded maximum size. Bytes read: ") + bytesRead;
      respCode = 102;
    }
    else if ( !pszRequestId || strlen(pszRequestId) == 0 )
    {
      writer.beginStringObj(respMsgKey);
      writer + F("Expected command to end with an integer request id (Format: {COMMAND}|{REQUEST_ID}, Example: GET|99).");
      writer + F(" Bytes read: ") + bytesRead;
      respCode = 103;
    }
    else
    {
      char *pszCmdName = strtok(pszCmd, ", \r\n");

      if ( !strcmp_P(pszCmdName,PSTR("VERSION")) )
      {
        writer.printlnStringObj(F("version"),VERSION,",")
          .printlnNumberObj(F("buildNumber"),BUILD_NUMBER,",")
          .printlnStringObj(F("buildDate"),BUILD_DATE,",")
          .beginStringObj(respMsgKey);
      }
      else if ( !strcmp_P(pszCmdName,PSTR("GET")) )
      {       
        writer.printlnNumberObj(F("fanMode"),Fan::mode,",");
        writer.printlnStringObj(F("fanModeText"),Fan::getFanModeText(Fan::mode),",");
        writer.printlnArrayObj(F("powerMeters"),powerMeters,",");
        writer.printlnArrayObj(F("devices"),devices,",");
        writer.beginStringObj(respMsgKey);
      } 
      else if ( !strcmp_P(pszCmdName,PSTR("SET_OUTPUT_FORMAT")) )
      {
        writer.beginStringObj(respMsgKey);
        
        const char* pszFormat = strtok(NULL,", ");
        const char* pszPersistMode = strtok(NULL,", ");
        PersistMode persistMode = ArduinoComponent::parsePersistMode(pszPersistMode);

        JsonFormat fmt = JSON_FORMAT_INVALID;
         
        if ( !strcmp_P(pszFormat,PSTR("JSON_COMPACT")) )
        {
          fmt = JSON_FORMAT_COMPACT;
        } 
        else if (!strcmp_P(pszFormat,PSTR("JSON_PRETTY")) )
        {
          fmt = JSON_FORMAT_PRETTY;
        }
        
        if ( !pszFormat || !pszPersistMode ) 
        {
          writer + F("Expected SET_OUTPUT_FORMAT,{JSON_COMPACT|JSON_PRETTY},{PERSIST|TRANSIENT} but found: ") + pszCmd;
          respCode = 201;
        }
        else if ( fmt == JSON_FORMAT_INVALID ) {
          writer + F("Expected JSON_COMPACT|JSON_PRETTY but found: ") + pszFormat;
          respCode = 202;
        }
        else if ( persistMode == PERSIST_MODE_INVALID ) 
        {
          writer + F("Expected PERSIST|TRANSIENT but found: ") + pszPersistMode;
          respCode = 203;
        }
        else
        {
          JsonSerialWriter::format = fmt;
          bool bPersist = persistMode == PERSIST_MODE_SAVE;
          if ( bPersist ) 
          {
            EEPROM.put( sizeof(VERSION)+sizeof(Fan::mode), JsonSerialWriter::format);
          }
          writer + "Output Format: " + pszFormat + ".  Saved to EEPROM: " + (bPersist?"TRUE":"FALSE");
        }
      }
      else if ( !strcmp_P(pszCmdName,PSTR("SET_FAN_MODE")) )
      {
        writer.beginStringObj(respMsgKey);
        
        const char* pszFanMode = strtok(NULL,", ");
        const char* pszPersistMode = strtok(NULL,", ");

        FanMode fanMode = FAN_MODE_INVALID;
        if ( !strcmp("ON",pszFanMode) ) {
          fanMode = FAN_MODE_ON;
        } else if ( !strcmp("OFF",pszFanMode) ) {
          fanMode = FAN_MODE_OFF;
        } else if ( !strcmp("AUTO",pszFanMode) ) {
          fanMode = FAN_MODE_AUTO;
        }

        PersistMode persistMode = ArduinoComponent::parsePersistMode(pszPersistMode);
        
        if ( !pszFanMode || !pszPersistMode ) 
        {
          writer + F("Expected SET_FAN_MODE,{ON|OFF|AUTO},{PERSIST|TRANSIENT} but found: ") + pszCmd;
          respCode = 301;
        }
        else if ( fanMode == FAN_MODE_INVALID ) 
        {
          writer + F("Expected ON|OFF|AUTO for fan mode. Received: ") + pszFanMode;
          respCode = 303;
        }
        else if ( persistMode == PERSIST_MODE_INVALID ) 
        {
          writer + F("Expected PERSIST|TRANSIENT but found: ") + pszPersistMode;
          respCode = 302;
        }
        else
        {
          Fan::mode = fanMode;
          bool bPersist = persistMode == PERSIST_MODE_SAVE;
          
          if ( bPersist )
          {
            EEPROM.put( sizeof(VERSION), Fan::mode);
          }
          writer + "Fan Mode: " + pszFanMode + ".  Saved to EEPROM: " + (bPersist?"TRUE":"FALSE");
        } 
      } 
      else if ( !strcmp_P(pszCmdName,PSTR("SET_FAN_THRESHOLDS")) )
      {
        writer.beginStringObj(respMsgKey);

        const char* pszDeviceFilter = strtok(NULL,",");
        const char* pszFanFilter = strtok(NULL,",");
        const char* pszOnTemp = strtok(NULL,",");
        const char* pszOffTemp = strtok(NULL,",");

        float onTemp = atof(pszOnTemp);
        float offTemp = atof(pszOffTemp);

        const char* pszPersistMode = strtok(NULL,", ");
        PersistMode persistMode = ArduinoComponent::parsePersistMode(pszPersistMode);

        if ( !pszDeviceFilter || !pszFanFilter || !pszOnTemp || !pszOffTemp || !pszPersistMode ) 
        {
          writer + F("Expected SET_FAN_THRESHOLDS,{device filter},{fan filter},{on temp},{off temp},{PERSIST|TRANSIENT} but found: ") + pszCmd;
          respCode = 401;
        }
        else if ( persistMode == PERSIST_MODE_INVALID ) 
        {
          writer + F("Expected PERSIST|TRANSIENT but found: ") + pszPersistMode;
          respCode = 402;
        }
        else if ( onTemp <= offTemp ) 
        {
          writer + F("Fan ON temperature must be greater than OFF temp.");            
          respCode = 403;
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
  
          if (updatedFans.length() == 0)
          {
            writer + F("No fans matched device and fan name filters");
            respCode = 404;
          }
          else 
          {
            // fans were updated
            bool bPersist = persistMode == PERSIST_MODE_SAVE;

            if ( bPersist )
            {
              for( int i = 0; devices[i] != NULL; i++ )
              {
                DeviceConfig deviceConfig(i,devices[i]);
                deviceConfig.save();
              }
            } 
            writer + F("Fan thresholds updated for: ") + updatedFans + ".  Saved to EEPROM: " + (bPersist?"TRUE":"FALSE");
          } 
        }
      }
      else if ( !strcmp_P(pszCmdName,PSTR("SET_POWER_METER")) )
      {
        //example: SET_POWER_METER,VCC,*,4.88,PERSIST
        //example: SET_POWER_METER,R1,*,1005000,PERSIST

        writer.beginStringObj(respMsgKey);
        
        const char* pszMemberName = strtok(NULL,",");
        const char* pszMeterNameFilter = strtok(NULL,",");
        const char* pszNewValue = strtok(NULL,", ");
        const char* pszPersistMode = strtok(NULL,", ");

        float newValue = atof(pszNewValue);
        
        int member = POWER_METER_MEMBER_INVALID;
        if ( !strcmp_P(pszMemberName,PSTR("VCC")) ) {    
          member = POWER_METER_MEMBER_VCC;
        } else if ( !strcmp_P(pszMemberName,PSTR("R1")) ) {
          member = POWER_METER_MEMBER_R1;
        } else if ( !strcmp_P(pszMemberName,PSTR("R2")) ) {
          member = POWER_METER_MEMBER_R2;
        } 
          
        PersistMode persistMode = ArduinoComponent::parsePersistMode(pszPersistMode);

        if ( !pszMemberName || !pszMeterNameFilter || !pszPersistMode || !pszNewValue ) 
        {
          writer + F("Expected SET_POWER_METER,{VCC|R1|R2},{meter name filter},{float value},{PERSIST|TRANSIENT} but found: ") + pszCmd;
          respCode = 501;
        }
        else if ( member == POWER_METER_MEMBER_INVALID )
        {
          writer + F("Expected VCC|R1|R2 but found: ") + pszMemberName;
          respCode = 502;
        } 
        else if ( persistMode == PERSIST_MODE_INVALID ) 
        {
          respCode = 503;
          writer + F("Expected PERSIST|TRANSIENT but found: ") + pszPersistMode;
        }
        else
        {
          //proceed... args okay
          String updatedMeters = "";
      
          for( int i = 0; powerMeters[i] != NULL; i++ )
          {
            if ( !strcmp(pszMeterNameFilter,"*") || strstr(powerMeters[i]->name,pszMeterNameFilter) )
            {
              switch(member) {
                case POWER_METER_MEMBER_R1:
                  powerMeters[i]->voltmeter->r1 = newValue;
                  break;
                case POWER_METER_MEMBER_R2:
                  powerMeters[i]->voltmeter->r2 = newValue;
                  break;
                case POWER_METER_MEMBER_VCC:
                default:
                  powerMeters[i]->voltmeter->vcc = newValue;
                  break;
              }

              if ( updatedMeters.length() )
              {
                updatedMeters += ",";
              }
              updatedMeters += powerMeters[i]->name;
            }
          }
          if ( updatedMeters.length() > 0 )
          {
            // meters were updated
            bool bPersist = persistMode == PERSIST_MODE_SAVE;

            if ( bPersist )
            {
              size_t baseAddr = PowerMeterConfig::computeFirstElementAddr(devices);
              
              for( int i = 0; powerMeters[i] != NULL; i++ )
              {
                PowerMeterConfig config(baseAddr,i,powerMeters[i]);
                config.save();
              }
            }
            writer + F("Set vcc=") + newValue + F(" for: ") + updatedMeters + ".  Saved to EEPROM: " + (bPersist?"TRUE":"FALSE");              
          } 
          else
          {
            writer + F("No power meter name matched filter: ") + pszMeterNameFilter;
            respCode = 504;
          }
        }
      }      
      else
      {
        writer.beginStringObj(respMsgKey)
          + F("Expected VERSION|GET|SET_FAN_MODE|SET_FAN_THRESHOLDS|SET_POWER_METER|SET_OUTPUT_FORMAT but found: ")
          + pszCmdName;
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

