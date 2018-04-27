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
// Prometheus gets arduino metrics using the Java "arduino-solar-client" in same
// parent folder as this project.
// 
//////////////////////////////////////////////////////////////////////////

//
// Change version if code changes impact data structures in EEPROM.
// A version change will reset EEPROM data to defaults.
//
#define VERSION "SOLAR-1.8"
#define BUILD_NUMBER 4
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

typedef unsigned char FanMode;

const FanMode FAN_MODE_OFF = 0, FAN_MODE_ON = 0x1, FAN_MODE_AUTO = 0x2;

FanMode fanMode = FAN_MODE_AUTO;

typedef unsigned char OutputFormat;

const OutputFormat JSON_COMPACT = 0, JSON_PRETTY = 1;

OutputFormat outputFormat = JSON_PRETTY;

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
    EEPROM.put(sizeof(VERSION), fanMode);
    EEPROM.put(sizeof(VERSION)+sizeof(fanMode), outputFormat);
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
    
    EEPROM.get(sizeof(VERSION), fanMode);

    EEPROM.get(sizeof(VERSION) + sizeof(fanMode), outputFormat);
    
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
  
  if ( bSerialAvailable || loopCnt++ % 20 == 0 )
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

      char *pszCmd = strtok(commandBuff, "|");            
      unsigned int requestId = atoi(strtok(NULL,"|"));
      char *pszCmdName = strtok(pszCmd, ", \r\n");

      while ( pszCmdName[0] == -1 ) {
        pszCmdName++;
      }
      
      int respCode = 0;
      String respMsg = "OK";

      JsonSerialWriter writer;
      JsonSerialWriter::clearByteCount();
      JsonSerialWriter::clearChecksum();
      writer.implPrint(F("#BEGIN:"));
      writer.implPrint(requestId);
      writer.implPrintln("#");
      writer.println( "{" );
      writer.increaseDepth();

      if ( !strcmp_P(pszCmdName,PSTR("VERSION")) )
      {
        writer.printlnStringObj(F("version"),VERSION,",")
          .printlnNumberObj(F("buildNumber"),BUILD_NUMBER,",")
          .printlnStringObj(F("buildDate"),BUILD_DATE,",");
      }
      else if ( !strcmp_P(pszCmdName,PSTR("GET")) )
      {       
        writer.printlnNumberObj(F("fanMode"),fanMode,",");
        writer.printlnStringObj(F("fanModeText"),Fan::getFanModeText(fanMode),",");
        writer.printlnArrayObj(F("powerMeters"),powerMeters,",");
        writer.printlnArrayObj(F("devices"),devices,",");
      } 
      else if ( !strcmp_P(pszCmdName,PSTR("SET_OUTPUT_FORMAT")) )
      {
        const char* pszFormat = strtok(NULL,", ");
        if ( !strcmp_P(pszFormat,PSTR("JSON_COMPACT")) )
        {
          outputFormat = JSON_COMPACT;
        } 
        else if (!strcmp_P(pszFormat,PSTR("JSON_PRETTY")) )
        {
          outputFormat = JSON_PRETTY;
        }
        else
        {
           respCode = 3;
           respMsg = F("Expected JSON_COMPACT|JSON_PRETTY but found: ");
           respMsg += pszFormat;
        }
        if ( respCode == 0 )
        {
          const char* pszSaveMode = strtok(NULL,", ");
          
          if ( !strcmp_P(pszSaveMode,PSTR("PERSIST")) )
          {
            EEPROM.put( sizeof(VERSION)+sizeof(fanMode), outputFormat);
            respMsg += " saved to EEPROM";
          } 
          else if ( !strcmp_P(pszSaveMode,PSTR("TRANSIENT")) )
          {
            // No action for transient
          } 
          else 
          {
            respCode = 301;
            respMsg = F("Expected PERSIST|TRANSIENT but found: ");
            respMsg += pszSaveMode;
          }
        }
      }
      else if ( !strcmp_P(pszCmdName,PSTR("SET_FAN_MODE")) )
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
          respCode = 302;
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
            respCode = 303;
            respMsg = F("Expected PERSIST|TRANSIENT but found: ");
            respMsg += pszSaveMode;
          }
        }
      } 
      else if ( !strcmp_P(pszCmdName,PSTR("SET_FAN_THRESHOLDS")) )
      {
        const char* pszDeviceFilter = strtok(NULL,",");
        const char* pszFanFilter = strtok(NULL,",");

        float onTemp = atof(strtok(NULL,", "));
        float offTemp = atof(strtok(NULL,", "));

        if ( onTemp <= offTemp ) 
        {
            respMsg = F("Fan ON temperature must be greater than OFF temp.");            
            respCode = 401;
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
              respCode = 403;
            }
          } 
          else
          {
            respMsg = F("No fans matched device and fan name filters");
            respCode = 404;
          }
        }
      }
      else if ( !strcmp_P(pszCmdName,PSTR("SET_POWER_METER")) )
      {
        //example: SET_POWER_METER,VCC,*,4.88,PERSIST
        //example: SET_POWER_METER,R1,*,1005000,PERSIST
        const char* pszMemberName = strtok(NULL,",");
        const char* pszMeterNameFilter = strtok(NULL,",");
        
        float newValue = atof(strtok(NULL,", "));

        String updatedMeters = "";
        
        for( int i = 0; powerMeters[i] != NULL; i++ )
        {
          if ( !strcmp(pszMeterNameFilter,"*") || strstr(powerMeters[i]->name,pszMeterNameFilter) )
          {
            if ( !strcmp_P(pszMemberName,PSTR("VCC")) ) {
              powerMeters[i]->voltmeter->vcc = newValue;
            } else if ( !strcmp_P(pszMemberName,PSTR("R1")) ) {
              powerMeters[i]->voltmeter->r1 = newValue;
            } else if ( !strcmp_P(pszMemberName,PSTR("R2")) ) {
              powerMeters[i]->voltmeter->r2 = newValue;
            } else {
              respMsg = F("Expected VCC|R1|R2 but found: ");
              respMsg += pszMemberName;
              respCode = 302;
              break;
            }
            
            if ( updatedMeters.length() )
            {
              updatedMeters += ",";
            }
            updatedMeters += powerMeters[i]->name;

          }
        }
  
        if ( respCode != 0 ) 
        {
        }
        else if ( updatedMeters.length() > 0 )
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
            respCode = 503;
          }
        } 
        else
        {
          respMsg = F("No power meter name matched filter: ");
          respMsg += pszMeterNameFilter;
          respCode = 504;
        }
      }      
      else
      {
        respMsg = F("Expected VERSION|GET|SET_FAN_MODE|SET_FAN_THRESHOLDS|SET_POWER_METER|SET_OUTPUT_FORMAT but found: ");
        respMsg += pszCmdName;
        respCode = 601;
      }
      
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
      writer.printlnNumberObj(F("respCode"),respCode,",");
      writer.printlnStringObj(F("respMsg"),respMsg);
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
    }
  } 
  delay(500);

 }

