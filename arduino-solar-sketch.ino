
#define VERSION "SOLAR-1.0"
#define BUILD_NUMBER 1
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
#include "arduino/CommandProcessor.h"
#include "arduino/Eeprom.h"
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
                 inverterTemp("Inverter Temp",5);

LightSensor lightLevel("Sunshine Level",6);                 
                
Dht dht(5);
DhtTempSensor enclosureTemp("Enclosure Temp",dht);
DhtHumiditySensor enclosureHumidity("Enclosure Humidity",dht);
VoltageSensor batteryBankVoltage("Battery Bank Voltage", 9, 1016000,101100);
VoltageSensor batteryBankAVoltage("Battery Bank A Voltage", 10, 1016000,101100);
vector<Sensor*> mainAndBankADelta { &batteryBankVoltage, &batteryBankAVoltage };
CompositeSensor batteryBankBVoltage("Battery Bank B Voltage", mainAndBankADelta, Sensor::delta);
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
  BooleanConstraint defaultOff {false};
  InverterSwitch() : arduino::PowerSwitch("Inverter Switch", 25, LOW) {
    setConstraint(&defaultOff);
  }
} inverterSwitch;

struct BatteryBankSwitch : public arduino::PowerSwitch {
  BooleanConstraint defaultOff {false};
  BatteryBankSwitch(const char* title, int pin) : arduino::PowerSwitch(title, pin, LOW) {
    setConstraint(&defaultOff);
  }
};
struct BatteryBankSwitch batteryBankASwitch("Battery Bank A Switch", 26), batteryBankBSwitch("Battery Bank B Switch", 27);

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
  Outlet2Switch() : OutletSwitch("Outlet 2 Switch", 27) {}
} outlet2Switch;


Devices devices {{ &exhaustFan, &chargerGroupFan, &inverterFan, &inverterSwitch, &batteryBankASwitch, &batteryBankBSwitch }};

Sensors sensors {{ 
    &chargerGroupTemp, 
    &batteryBankVoltage, &batteryBankCurrent, &batteryBankPower,
    &batteryBankAVoltage,&batteryBankBVoltage,
    &charger1Temp, &charger2Temp, &inverterTemp, &sunroomTemp, &atticTemp, 
    &enclosureTemp, &enclosureHumidity, 
    &exhaustFan.toggleSensor, &chargerGroupFan.toggleSensor, 
    &inverterFan.toggleSensor, &inverterSwitch.toggleSensor,
    &outlet1Switch.toggleSensor, &outlet2Switch.toggleSensor,
    &lightLevel
}};

void setup() {
  //Serial.begin(38400, SERIAL_8O1); // bit usage: 8 data, odd parity, 1 stop
  Serial.begin(57600);

  String version;
  eeprom.getVersion(version);
  if ( version != VERSION )
  {
    gLastInfoMsg = F("Version changed. Clearing EEPROM and saving defaults.  Old: ");
    gLastInfoMsg += version;
    gLastInfoMsg += " New: ";
    gLastInfoMsg += VERSION;   
    eeprom.setVersion(VERSION);
    eeprom.setJsonFormat(arduino::jsonFormat);
    eeprom.setCommandCount(0);
  }
  else
  {
    gLastInfoMsg = F("Loading EEPROM data for version ");
    gLastInfoMsg += version;
    arduino::jsonFormat = eeprom.getJsonFormat();
    StringStreamWriter writer;
    CommandProcessor<StringStreamWriter>::setup(writer,sensors,devices);
    cout << writer.serial.ss.str() << endl;
  }
  
  for(Sensor* pSensor : sensors) {
    pSensor->setup();
  }
  for(Device* pDevice : devices) {
    pDevice->setup();
  }

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
    for(Device* pDevice : devices) {
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
     
    JsonSerialWriter writer;
    JsonSerialWriter::clearByteCount();
    JsonSerialWriter::clearChecksum();
    writer.implPrint(F("#BEGIN:"));
    writer.implPrint(requestId);
    writer.implPrintln("#");
    writer.println( "{" );
    writer.increaseDepth();
    CommandProcessor<JsonSerialWriter> cmdProcessor(writer,sensors,devices);

    
    if ( msgReadTimedOut ) 
    {
      cmdProcessor.beginResp();
      writer + F("Serial data read timed out.  Bytes received: ") + bytesRead;
      cmdProcessor.endResp(101);
    }
    else if ( msgSizeExceeded )
    {
      cmdProcessor.beginResp();
      writer + F("Request exceeded maximum size. Bytes read: ") + bytesRead;
      cmdProcessor.endResp(102);
    }
    else
    {
      cmdProcessor.execute(pszCmd);      
    }
    
    writer.decreaseDepth();
    writer.print("}");
    writer.implPrint(F("\n#END:"));
    writer.implPrint(requestId);
    writer.implPrint(":");
    writer.implPrint(JsonSerialWriter::getByteCount());
    writer.implPrint(":");
    writer.implPrint(JsonSerialWriter::getChecksum());
    writer.implPrintln("#");

    bytesRead = 0; // reset commandBuff
    beginCmdReadTimeMs = 0;
  }    
}
