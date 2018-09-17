#ifndef ARDUINO_SOLAR_SKETCH_COMMANDPROCESSOR_H
#define ARDUINO_SOLAR_SKETCH_COMMANDPROCESSOR_H


#include "Arduino.h"
#include "JsonWriter.h"
#include "Eeprom.h"

#include "../automation/capability/Capability.h"

#include <vector>
#include <string>

using namespace std;

namespace arduino {


  template<typename T>
  class CommandProcessor {

  public:

    T &writer;
    Sensors &sensors;
    Devices &devices;

    static void setup(T &writer, Sensors &sensors, Devices &devices) {

      // run script lines from EEPROM
      CommandProcessor<T> cmdProcessor(writer,sensors,devices);
      cmdProcessor.runSetup();
    }

    CommandProcessor(T &writer, Sensors &sensors, Devices &devices)
        : writer(writer),
          sensors(sensors),
          devices(devices) {
    }

    void runSetup() {
      writer + "{\n";
      int cmdCnt = eeprom.getCommandCount();
      for ( int i = 0; i < cmdCnt; i++ ) {
        String cmd;
        int respCode = eeprom.getCommandAt(i,cmd);
        if ( respCode == CMD_OK ) {
          char cmdBuff[CMD_ARR_MAX_ROW_LENGTH];
          strcpy(cmdBuff, cmd.c_str());
          respCode = executeLine(cmdBuff);
        }
      }
      writer + "}\n";
    }

    T &beginResp() {
      writer.beginStringObj(F("respMsg"));
      return writer;
    }

    T &endResp(int respCode) {
      if ( writer.getOpenStringValByteCnt() <= 0 ) {
        writer + (respCode == 0 ? F("OK") : F("ERROR"));
      }
      writer.endStringObj(",");
      writer.noPrefixPrintln("");

      if ( gLastErrorMsg.length() )
      {
        if ( respCode == 0 ) {
          respCode = -1;
        }
        writer.printlnStringObj(F("LastErrorMsg"),gLastErrorMsg,",");
        gLastErrorMsg = "";
      }
      else if ( gLastInfoMsg.length() )
      {
        writer.printlnStringObj(F("LastInfoMsg"),gLastInfoMsg,",");
        gLastInfoMsg = "";
      }
      writer.printlnNumberObj(F("respCode"), respCode);
      return writer;
    }

    int execute(char *pszScript) {
      int cmdResult = 0;
      int cmdCnt = 0;

      size_t scriptLen = strlen(pszScript);
      char *pszCmd = strtok(pszScript, "\r\n");
      size_t nextIndex = 0;

      while (pszCmd) {
        cmdCnt++;
        int cmdLen = strlen(pszCmd); // need length before strtok alters content
        int cmdResult = executeLine(pszCmd);
        if (cmdResult) {
          break;
        }
        nextIndex += cmdLen + 1;
        if ( nextIndex >= scriptLen ) {
          break;
        } else {
          pszCmd = strtok(&pszScript[nextIndex], "\r\n");
        }
      }
      return cmdResult;
    }

    int executeLine(char *pszCmd) {
      int respCode = 0;
      //cout << __PRETTY_FUNCTION__ << " command: " << pszCmd << "." << endl;
      char *pszCmdName = strtok(pszCmd, ", ");
      if (!strcmp_P(pszCmdName, PSTR("SETUP"))) {
        respCode = processSetupCommand();
      } else if (!strcmp_P(pszCmdName, PSTR("SET"))) {
        respCode = processSetCommand();
      } else if (!strcmp_P(pszCmdName, PSTR("GET"))) {
        respCode = processGetCommand();
      } else {
        beginResp() + F("Expected {GET|SET|SETUP} but found: ") + pszCmdName;
        endResp(UNEXPECTED_CMD_ARGUMENT);
      }
      return respCode;
    }


  protected:

    int processSetupCommand() {
      int respCode = 0;
      const char *pszAction = strtok(NULL, ", \r\n");

      beginResp();

      if (!strcmp_P(pszAction, PSTR("ADD"))) {
        const char *pszCmd = strtok(NULL,"\r\n");
        respCode = eeprom.addCommand(pszCmd);
        if ( respCode != CMD_OK ) {
          writer + errorDesc(F("SETUP,ADD"),respCode);
        }
      } else if (!strcmp_P(pszAction, PSTR("REPLACE_AT"))) {
        const char* pszIndex = strtok(NULL, ",\r\n");
        const char* pszCmd = strtok(NULL, "\r\n");
        int index = atoi(pszIndex);
        respCode = eeprom.setCommandAt(index,pszCmd);
        if ( respCode != CMD_OK ) {
          writer + errorDesc(F("SETUP,REPLACE_AT"),respCode);
        }
      } else if (!strcmp_P(pszAction, PSTR("REMOVE"))) {
        const char* pszCmd = strtok(NULL, "\r\n");
        respCode = eeprom.removeCommand(pszCmd);
        if ( respCode != CMD_OK ) {
          writer + errorDesc(F("SETUP,REMOVE"),respCode);
        }
      } else if (!strcmp_P(pszAction, PSTR("REMOVE_AT"))) {
        const char* pszIndex = strtok(NULL, ",\r\n");
        int index = atoi(pszIndex);
        respCode = eeprom.removeCommandAt(index);
        if ( respCode != CMD_OK ) {
          writer + errorDesc(F("SETUP,REMOVE_AT"),respCode);
        }
      } else if (!strcmp_P(pszAction, PSTR("RUN"))) {
        const char* pszIndex = strtok(NULL, ",\r\n");
        int index = atoi(pszIndex);
        respCode = eeprom.removeCommandAt(index);
        if ( respCode != CMD_OK ) {
          writer + errorDesc(F("SETUP,REMOVE_AT"),respCode);
        }
      } else {
        writer + F("SETUP expected {ADD|REPLACE_AT|REMOVE|REMOVE_AT|RUN} but found: '") + pszAction + "'.";
        respCode = UNEXPECTED_CMD_ARGUMENT;
      }
      endResp(respCode);
      return respCode;
    }

    int processGetCommand() {
      int respCode = 0;
      const char *pszArg;
      while ((pszArg = strtok(NULL, ", ")) != nullptr) {
        if (!strcmp_P(pszArg, PSTR("ENV"))) {
          writer.printKey("env");
          writer.println("{");
          writer.increaseDepth();
          writer.printlnStringObj(F("release"), VERSION, ",")
              .printlnNumberObj(F("buildNumber"), BUILD_NUMBER, ",")
              .printlnStringObj(F("buildDate"), BUILD_DATE, ",")
              .printlnStringObj(F("Vcc"), readVcc(), ",")
              .beginStringObj("time");
          time_t t = now();
          writer + year(t) + "-" + month(t) + "-" + day(t) + " " + hour(t) + ":" + minute(t) + ":" + second(t);
          writer.endStringObj();
          writer.noPrefixPrintln(",");
          writer.printlnStringObj("timeSet", timeStatus() == timeSet ? "YES" : "NO");
          writer.decreaseDepth();
          writer.println("},");
        } else if (!strcmp_P(pszArg, PSTR("SETUP"))) {
          eeprom.print(1);
          writer.noPrefixPrintln(",");
        } else if (!strcmp_P(pszArg, PSTR("OUTPUT_FORMAT"))) {
          writer.printlnStringObj(F("outputFormat"), formatAsString(arduino::jsonFormat), ",");
        } else if (!strcmp_P(pszArg, PSTR("SENSORS"))) {
          const char* pszCommaDelimitedNames = strtok(NULL,"\r\n");
          Sensors filteredSensors;
          sensors.filterByNames(pszCommaDelimitedNames,filteredSensors);
          writer.printlnVectorObj(F("sensors"), filteredSensors, ",");
        } else if (!strcmp_P(pszArg, PSTR("DEVICES"))) {
          const char* pszCommaDelimitedNames = strtok(NULL,"\r\n");
          Devices filteredDevices;
          devices.filterByNames(pszCommaDelimitedNames,filteredDevices);
          writer.printlnVectorObj(F("devices"), filteredDevices, ",");
        } else if (!strcmp_P(pszArg, PSTR("TIME"))) {
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
        } else {
          beginResp();
          writer + F("GET command expected {SENSORS|DEVICES|OUTPUT_FORMAT|TIME|ENV|SETUP} but found: '") + pszArg + "'.";
          respCode = UNEXPECTED_CMD_ARGUMENT;
        }
      }
      if (!respCode) {
        beginResp();
        writer + "OK";
      }
      endResp(respCode);
      return respCode;
    }

    int processSetCommand() {
      int respCode = 0;

      beginResp();
      const char *pszArg = strtok(NULL, ", ");

      if (!strcmp_P(pszArg, PSTR("OUTPUT_FORMAT"))) {
        const char *pszFormat = strtok(NULL, ", \r\n");
        if (!strcmp_P(pszFormat, PSTR("JSON_COMPACT"))) {
          arduino::jsonFormat = JSON_FORMAT_COMPACT;
        } else if (!strcmp_P(pszFormat, PSTR("JSON_PRETTY"))) {
          arduino::jsonFormat = JSON_FORMAT_PRETTY;
        } else {
          writer + F("Expected JSON_COMPACT|JSON_PRETTY but found: ") + pszFormat;
          respCode = 301;
        }

      } else if (!strcmp_P(pszArg, PSTR("TIME_T"))) {
        const char *pszDateTime = strtok(NULL, ", ");
        time_t time = atol(pszDateTime);
        setTime(time);
        writer + F("TIME_T set to ") + time;

      } else if (!strcmp_P(pszArg, PSTR("TIME"))) {

        int year = atoi(strtok(NULL, ", ")), month = atoi(strtok(NULL, ", ")), day = atoi(strtok(NULL, ", ")),
            hour = atoi(strtok(NULL, ", ")), minute = atoi(strtok(NULL, ", ")), second = atoi(strtok(NULL, ", "));
        setTime(hour, minute, second, day, month, year);
        writer + F("TIME set using YYYY,MM,DD,HH,mm,SS args");

      } else if (!strcmp_P(pszArg, PSTR("DEVICE_CAPABILITY"))) {
        const char *pszDeviceName = strtok(NULL, ",\r\n");
        const char *pszCapabilityType = strtok(NULL, ", \r\n");
        stringstream ss;
        bool bUpdated = false;
        const char *pszTargetState = strtok(NULL, ", \r\n");
        Devices filteredDevices;
        devices.filterByNames(pszDeviceName,filteredDevices);
        for (Device *pDevice : filteredDevices) {
          bUpdated |= findAndSetCapability(pDevice->capabilities, pszCapabilityType, pszTargetState, ss);
        }
        if (bUpdated) {
          writer + ss.str().c_str();
        }
      } else if (!strcmp_P(pszArg, PSTR("DEVICE_MODE"))) {
        const char *pszDeviceName = strtok(NULL, ",\r\n");
        const char *pszMode = strtok(NULL, ", \r\n"); // OFF,ON,AUTO... i.e.  a cooling fan
                                                      // will always be off, on, or applyConstraint will determine on/off
                                                      // dynamically based on temperature sensors child constraints.
        Devices filteredDevices;
        devices.filterByNames(pszDeviceName,filteredDevices);
        stringstream ss;
        bool bUpdated = false;
        for (Device *pDevice: filteredDevices ) {
          if (pDevice->pConstraint) {
            Constraint::Mode mode = Constraint::parseMode(pszMode);
            pDevice->pConstraint->mode = mode;
            pDevice->applyConstraint();
            if (bUpdated) {
              ss << ", ";
            } else {
              bUpdated = true;
            }
            ss << pDevice->name << ": " << Constraint::modeToString(pDevice->pConstraint->mode);
          }
        }
        if (bUpdated) {
          writer + ss.str().c_str();
        } else {
          writer + F("No device mode updates.  Device name filter: ");
          writer + pszDeviceName;
        }
      } else if (!strcmp_P(pszArg, PSTR("SETTING"))) {
        const char *pszSettingName = strtok(NULL, ",\r\n");
        const char *pszSettingValue = strtok(NULL, ",\r\n");

      } else {
        writer + F("Expected TIME_T|TIME|DEVICE_MODE|DEVICE_CAPABILITY|SETTING but found: ") + pszArg;
        respCode = UNEXPECTED_CMD_ARGUMENT;
      }
      endResp(respCode);
      return respCode;
    }

    bool
    findAndSetCapability(vector<Capability *> &capabilities, const char *pszCapabilityType, const char *pszTargetState,
                         ostream &respMsgStream) {
      float targetValue = !strcasecmp(pszTargetState, "on") ? 1 : !strcasecmp(pszTargetState, "off") ? 0 : atof(
          pszTargetState);
      bool bUpdated = false;
      for (auto cap : capabilities) {
        if (cap->getType() == pszCapabilityType) {
          cap->setValue(targetValue);
          if (!bUpdated) {
            respMsgStream << ",";
            bUpdated = true;
          }
          respMsgStream << cap->getTitle() << ":" << cap->getValue();
        }
      }
      return bUpdated;
    }

  };

}
#endif //ARDUINO_SOLAR_SKETCH_COMMANDPROCESSOR_H
