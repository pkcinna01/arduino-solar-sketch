#define ARDUINO_APP
#define VERSION "SOLAR-1.45"
#define BUILD_NUMBER 2
#define BUILD_DATE __DATE__

#include <EEPROM.h>
#include <Wire.h>
#include <ArduinoSTL.h>

// automation includes not specific to arduino
#include "automation/Automation.h"
#include "automation/json/JsonStreamWriter.h"
#include "automation/sensor/Sensor.h"
#include "automation/sensor/CompositeSensor.h"
#include "automation/constraint/Constraint.h"
#include "automation/constraint/NotConstraint.h"
#include "automation/constraint/ValueConstraint.h"
#include "automation/constraint/TimeRangeConstraint.h"
#include "automation/constraint/AndConstraint.h"
#include "automation/sensor/Sensor.cpp"
#include "automation/device/Device.cpp"
#include "automation/capability/Capability.cpp"
#include "automation/constraint/Constraint.cpp"

using namespace automation;

#include "arduino/Arduino.h"
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

#include <vector>
#include <sstream>
#include <algorithm>
#include <string.h>

using namespace std;
using namespace arduino;

const uint8_t commandBuffSize = 200;
static char commandBuff[commandBuffSize+1];

// PMSTR is used to initialize names from PROGMEM to save runtime RAM space
// PROGMEM needs to be in scope of a function so using a lambda to create a local temp area
#define PMSTR(name) []() -> char* { static const char x[] PROGMEM = {name}; \
  strncpy_P(commandBuff,x,commandBuffSize); commandBuff[commandBuffSize] = '\0'; \
  return (char*) commandBuff; }()

ThermistorSensor charger1Temp(PMSTR("Charger 1 Temp"), 0),
                 charger2Temp(PMSTR("Charger 2 Temp"), 1),
                 //sunroomTemp(PMSTR("Sunroom Temp"), 2),
                 //atticTemp(PMSTR("Attic Temp"), 3),
                 inverterTemp(PMSTR("Inverter Temp"), 4);

//LightSensor lightLevel(PMSTR("Sunshine"), 5);

Dht dht(5);
//DhtTempSensor enclosureTempDht(PMSTR("Enclosure Temp (DHT)"), dht); // not enough power to have DHT running and all relays
ThermistorSensor enclosureTemp(PMSTR("Enclosure Temp"), 6);
//DhtHumiditySensor enclosureHumidityDht(PMSTR("Enclosure Humidity"), dht);
VoltageSensor batteryBankVoltage(PMSTR("Battery Bank Voltage"), 9, 1016000, 101100);
VoltageSensor batteryBankBVoltage(PMSTR("Bank B Voltage"), 10, 1016000, 101100);
vector<Sensor*> mainAndBankBDelta { &batteryBankVoltage, &batteryBankBVoltage };
CompositeSensor batteryBankAVoltage(PMSTR("Bank A Voltage"), mainAndBankBDelta, Sensor::delta);
CurrentSensor batteryBankCurrent(PMSTR("Bank Current"));
PowerSensor batteryBankPower(PMSTR("Battery Bank Power"), &batteryBankVoltage, &batteryBankCurrent);
vector<Sensor*> chargerGrpSensors { &charger1Temp, &charger2Temp };
CompositeSensor chargerGroupTemp(PMSTR("Chargers Temp"), chargerGrpSensors, Sensor::maximum);
//vector<Sensor*> enclosureGrpSensors { &enclosureTempDht, &enclosureTemp };
//CompositeSensor enclosureGroupTemp(PMSTR("Enclosure Temp"), enclosureGrpSensors, Sensor::maximum);
vector<Sensor*> inverterGrpSensors { &enclosureTemp, &inverterTemp };
CompositeSensor inverterGroupTemp(PMSTR("Inverter and Enclosure Temp"), inverterGrpSensors, Sensor::maximum);

arduino::CoolingFan exhaustFan(PMSTR("Enclosure Fan"), 22, enclosureTemp, 95, 90, LOW);
arduino::CoolingFan chargerGroupFan(PMSTR("Chargers Fan"), 23, chargerGroupTemp, 110, 105, LOW);
arduino::CoolingFan inverterFan(PMSTR("Inverter Fan"), 24, inverterGroupTemp, 105, 100, LOW);

struct MinBatteryBankVoltage : AtLeast<float, Sensor&> {
  MinBatteryBankVoltage(float volts) : AtLeast(volts, batteryBankVoltage) {
  }
};

struct MinBatteryBankVoltage outletsMinSteadySupplyVoltage(23);
struct MinBatteryBankVoltage outletsMinDipSupplyVoltage(21.5);

struct InverterSwitch : public arduino::PowerSwitch {
  BooleanConstraint defaultOff {false};
  InverterSwitch() : arduino::PowerSwitch(PMSTR("Inverter Disconnect"), 25, LOW) {
    defaultOff.mode = Constraint::REMOTE_MODE;
    setConstraint(&defaultOff);
  }
  RTTI_GET_TYPE_IMPL(main, InverterSwitch)
} inverterSwitch;

/*struct BatteryBankSwitch : public arduino::PowerSwitch {
  BooleanConstraint defaultOff {false};
  BatteryBankSwitch(const char* title, int pin) : arduino::PowerSwitch(title, pin, LOW) {
    setConstraint(&defaultOff);
  }
  RTTI_GET_TYPE_IMPL(main, BatteryBankSwitch)
} batteryBankASwitch(PMSTR("Bank A Switch"), 26), batteryBankBSwitch("Bank B Switch", 27);
*/
struct OutletSwitch : public arduino::PowerSwitch {
  AndConstraint constraints { {&outletsMinSteadySupplyVoltage, &outletsMinDipSupplyVoltage} };
  OutletSwitch(const string& name, int pin, int onValue = HIGH) : arduino::PowerSwitch(name, pin, onValue) {
    setConstraint(&constraints);
    outletsMinDipSupplyVoltage.setPassDelayMs(5ul * MINUTES).setFailDelayMs(15 * SECONDS).setPassMargin(2);
    outletsMinSteadySupplyVoltage.setPassDelayMs(15ul * MINUTES).setFailDelayMs(1 * MINUTES).setPassMargin(2);
  }
};

struct Outlet1Switch : public OutletSwitch {
  Outlet1Switch() : OutletSwitch(PMSTR("Outlet 1"), 30) {}
} outlet1Switch;

struct Outlet2Switch : public OutletSwitch {
  Outlet2Switch() : OutletSwitch(PMSTR("Outlet 2"), 31) {}
} outlet2Switch;


Devices devices {{
    &exhaustFan, &chargerGroupFan, &inverterFan,
    &inverterSwitch,
    //&batteryBankASwitch, &batteryBankBSwitch,
    &outlet1Switch, &outlet2Switch
  }};

Sensors sensors {{
    &chargerGroupTemp,
    &batteryBankVoltage, &batteryBankCurrent, &batteryBankPower,
    &batteryBankAVoltage, &batteryBankBVoltage,
    &enclosureTemp, //&atticTemp, &enclosureGroupTemp,
    //&enclosureTempDht, &enclosureHumidityDht,
    &charger1Temp, &charger2Temp, &inverterTemp, &inverterGroupTemp, //&sunroomTemp,
    &exhaustFan.toggleSensor, &chargerGroupFan.toggleSensor,
    &inverterFan.toggleSensor, &inverterSwitch.toggleSensor,
    //&batteryBankASwitch.toggleSensor, &batteryBankBSwitch.toggleSensor,
    &outlet1Switch.toggleSensor, &outlet2Switch.toggleSensor//, &lightLevel
  }};

void setup() {

  unsigned long serialSpeed = 38400;
  //unsigned long serialSpeed = 57600;
  //unsigned int serialConfig = SERIAL_8O1;
  unsigned int serialConfig = SERIAL_8N1;
  
  String version;
  eeprom.getVersion(version);
  if ( version != VERSION )
  {
    gLastInfoMsg = F("SETUP: Version changed. Clearing EEPROM and saving defaults.  Old: ");
    gLastInfoMsg += version;
    gLastInfoMsg += F(" New: ");
    gLastInfoMsg += VERSION;
    eeprom.setVersion(VERSION);
    eeprom.setSerialSpeed(serialSpeed);
    eeprom.setSerialConfig(serialConfig);
    eeprom.setJsonFormat(automation::json::jsonFormat);
    eeprom.setCommandCount(0);
  }
  else
  {
    gLastInfoMsg = F("SETUP: Loading EEPROM data for version ");
    gLastInfoMsg += version;
    json::jsonFormat = eeprom.getJsonFormat();
    serialSpeed = eeprom.getSerialSpeed();
    serialConfig = eeprom.getSerialConfig();
    JsonStreamWriter writer(json::nullStreamPrinter);
    CommandProcessor::setup(writer, sensors, devices);
  }

  Serial.begin(serialSpeed, serialConfig);

  for (Sensor* pSensor : sensors) {
    pSensor->setup();
  }
  for (Device* pDevice : devices) {
    pDevice->setup();
  }

  arduino::watchdog::enable();
  commandBuff[0] = '\0';
}

void loop() {
  static unsigned long lastUpdateTimeMs = 0, beginCmdReadTimeMs = 0;
  static unsigned int updateIntervalMs = 15000;
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
    } else if ( bytesRead >= commandBuffSize ) {
      commandBuff[bytesRead - 1] = '\0';
      msgSizeExceeded = true;
      break;
    }
  }

  if ( (cmdReady && strlen(commandBuff)) || (currentTimeMs - lastUpdateTimeMs) > updateIntervalMs )
  {
    sensors.reset(); // clear cached values
    sensors.getValuesBySampling(); // save time by sampling in parallel
    
    if ( !Constraints::isPaused() ) {      
      for (Device* pDevice : devices) {
        bool bIgnoreSameResult = false; // this will override remote changes if constraint mode is not REMOTE
        pDevice->applyConstraint(bIgnoreSameResult);
      }
    }
    
    lastUpdateTimeMs = millis();
  } else if ( beginCmdReadTimeMs > 0 && (currentTimeMs - beginCmdReadTimeMs) > updateIntervalMs ) {
    msgReadTimedOut = true;
  }

  if ( cmdReady || msgReadTimedOut ) {

    char *pszCmd = strtok(commandBuff, "|");
    char *pszRequestId = strtok(NULL, "|");
    unsigned int requestId = atoi(pszRequestId);

    JsonSerialWriter writer;
    writer.clearByteCount();
    writer.clearChecksum();
    writer.implPrint(F("#BEGIN:"));
    writer.implPrint(requestId);
    writer.implPrintln("#");
    writer.println( "[" );
    writer.increaseDepth();
    CommandProcessor cmdProcessor(writer, sensors, devices);

    if ( msgReadTimedOut )
    {
      writer.println("{");
      cmdProcessor.beginResp();
      writer + F("Serial data read timed out.  Bytes received: ") + bytesRead;
      if ( bytesRead == 1 ) {
        writer + F(". First byte: ") + ((int)commandBuff[0]);
      }
      cmdProcessor.endResp(101);
      writer.println("}");
    }
    else if ( msgSizeExceeded )
    {
      writer.println("{");
      cmdProcessor.beginResp();
      writer + F("Request exceeded maximum size. Bytes read: ") + bytesRead;
      cmdProcessor.endResp(102);
      writer.println("}");
    }
    else
    {
      arduino::watchdog::keepAlive();
      automation::client::watchdog::messageReceived();
      cmdProcessor.execute(pszCmd);
    }

    writer.decreaseDepth();
    writer.print("]");
    writer.implPrint(F("\n#END:"));
    writer.implPrint(requestId);
    writer.implPrint(":");
    writer.implPrint(writer.getByteCount());
    writer.implPrint(":");
    writer.implPrint(writer.getChecksum());
    writer.implPrint(":");
    writer.implPrint(automation::isTimeValid()?1:0);
    writer.implPrint(":");
    writer.implPrint(eeprom.getDeviceId());
    writer.implPrintln("#");

    bytesRead = 0; // reset commandBuff
    beginCmdReadTimeMs = 0;
  }

  arduino::watchdog::keepAlive();
}
