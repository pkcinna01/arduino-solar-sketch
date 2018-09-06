//
// Change version if code changes impact data structures in EEPROM.
// A version change will reset EEPROM data to defaults.
//
#define VERSION "SOLAR-1.8"
#define BUILD_NUMBER 7
#define BUILD_DATE __DATE__

#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>
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

#include "arduino/JsonWriter.h"
#include "arduino/Dht.h"
#include "arduino/DhtHumiditySensor.h"
#include "arduino/DhtTempSensor.h"
#include "arduino/ThermistorSensor.h"
#include "arduino/VoltageSensor.h"
#include "arduino/CurrentSensor.h"
#include "arduino/PowerSensor.h"
#include "arduino/CoolingFan.h"
#include "arduino/PowerSwitch.h"
#include "arduino/Automation.cpp"
#include "arduino/Sensor.cpp"

#include <vector>
#include <sstream>

using namespace std;
using namespace arduino;

ThermistorSensor charger1Temp("Charger 1 Temp",0),
                 charger2Temp("Charger 2 Temp",1),
                 sunroomTemp("Sunroom Temp",2),
                 atticTemp("Attic Temp",3),
                 inverterTemp("Inverter Temp",4);
Dht dht(5);
DhtTempSensor enclosureTemp("Enclosure Temp",dht);
DhtHumiditySensor enclosureHumidity("Enclosure Humidity",dht);
VoltageSensor batteryBankVoltage("Battery Bank Voltage", 7);
CurrentSensor batteryBankCurrent("Battery Bank Current");
PowerSensor batteryBankPower("Battery Bank Power", &batteryBankVoltage, &batteryBankCurrent);
vector<Sensor*> chargerGrpSensors { &charger1Temp, &charger2Temp };
CompositeSensor chargerGroupTemp("Charger Group Temp", chargerGrpSensors, Sensor::maximum);

//TimeRangeConstraint chargerGroupFanEnabledTimeRange( { 8, 0, 0 }, { 18, 30, 0 } );

arduino::CoolingFan exhaustFan("Enclosure Exhaust Fan", 22, enclosureTemp, 90, 89,LOW);
arduino::CoolingFan chargerGroupFan("Charger Group Fan", 23, chargerGroupTemp, 90, 89,LOW);
arduino::CoolingFan inverterFan("Inverter Fan", 24, inverterTemp, 90, 89,LOW);
arduino::PowerSwitch inverterPower("Inverter Power", 25,LOW);

Device* devices[] = { 
    &exhaustFan, &chargerGroupFan, &inverterFan, &inverterPower, nullptr
};
Sensor* sensors[] = { 
    &chargerGroupTemp, 
    &batteryBankVoltage, &batteryBankCurrent, &batteryBankPower,
    &charger1Temp, &charger2Temp, &inverterTemp, &sunroomTemp, &atticTemp, 
    &enclosureTemp, &enclosureHumidity, 
    &exhaustFan.toggleSensor, &chargerGroupFan.toggleSensor, 
    &inverterFan.toggleSensor, &inverterPower.toggleSensor,
    nullptr
};

void setup() {
  //Serial.begin(38400, SERIAL_8O1); // bit usage: 8 data, odd parity, 1 stop
  Serial.begin(57600);
  cout << endl << "=================================================================" << endl << __PRETTY_FUNCTION__ << endl;
  //static AndConstraint checkTimeAndDefaults({&chargerGroupFanEnabledTimeRange, chargerGroupFan.getConstraint()});
  //chargerGroupFan.setConstraint(&checkTimeAndDefaults);
  //sensors.push_back( &exhaustFan.toggleSensor );
  //sensors.push_back( &chargerGroupFan.toggleSensor );

  for(int i=0; sensors[i] != nullptr; i++) {
    cout << __PRETTY_FUNCTION__ << " sensor " << sensors[i]->name << " setup..." << endl;
    sensors[i]->setup();
  }
  for(int i=0; devices[i] != nullptr; i++) {
    cout << __PRETTY_FUNCTION__ << " device " << devices[i]->name << " setup..." << endl;
    devices[i]->setup();
  }
  cout << __PRETTY_FUNCTION__ << " complete" << endl;
}

void loop() {
  static unsigned long lastUpdateTimeMs = 0, beginCmdReadTimeMs = 0;
  static unsigned int updateIntervalMs = 1000;
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
    
  if ( cmdReady ) //|| (currentTimeMs - lastUpdateTimeMs) > updateIntervalMs )
  {
    lastUpdateTimeMs = currentTimeMs;
    cout << ">>> Sensors <<<" << endl;
    for(int i=0; sensors[i] != nullptr; i++) {
      sensors[i]->print();
      //cout << *sensors[i];
      cout << endl;
    }    

    cout << ">>> Devices <<<" << endl;
    for(int i=0; devices[i] != nullptr; i++) {
      Device* pDevice = devices[i];
      cout << endl << pDevice->name << ": " << endl;
      if ( pDevice->getConstraint() ) {
        cout << "\t" << pDevice->getConstraint()->getTitle();
        cout << " = " << (pDevice->getConstraint()->test() ? "PASSED" : "FAILED") << endl;
        pDevice->applyConstraint(false);
      } 
    }
    if ( gLastErrorMsg.length() ) {
      cout << "ERRORS: " << gLastErrorMsg << endl;
    }
  } 
  else if ( beginCmdReadTimeMs > 0 && (currentTimeMs - beginCmdReadTimeMs) > updateIntervalMs ) 
  {
    msgReadTimedOut = true;
  }
  // clear last error AFTER response sent to save memory
  gLastErrorMsg = "";    
  gLastInfoMsg = "";

  bytesRead = 0; // reset commandBuff
  beginCmdReadTimeMs = 0;
}
