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

    int runSetup() {
      int cmdCnt = eeprom.getCommandCount();
      for ( int i = 0; i < cmdCnt; i++ ) {
        String cmd;
        if ( i > 0 ) {
          writer.noPrefixPrintln(",");
        }
        int respCode = eeprom.getCommandAt(i,cmd);
        if ( respCode == CMD_OK ) {
          char cmdBuff[CMD_ARR_MAX_ROW_LENGTH];
          strcpy(cmdBuff, cmd.c_str());
          respCode = executeLine(cmdBuff);
          if ( respCode != CMD_OK ) {
            return respCode;
          }
        }
      }
      writer.println();
      return CMD_OK;
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
        writer.printlnStringObj(F("lastErrorMsg"),gLastErrorMsg,",");
        gLastErrorMsg = "";
      }
      if ( gLastInfoMsg.length() )
      {
        writer.printlnStringObj(F("lastInfoMsg"),gLastInfoMsg,",");
        gLastInfoMsg = "";
      }
      writer.printlnNumberObj(F("respCode"), respCode);
      return writer;
    }

    int execute(char *pszScript) {
      int cmdResult = 0;
      int cmdCnt = 0;

      size_t scriptLen = strlen(pszScript);
      char *pszCmd = strtok(pszScript, ";\r\n");
      size_t nextIndex = 0;

      while (pszCmd) {
        int cmdLen = strlen(pszCmd); // need length before strtok alters content
        if ( cmdCnt++ > 0 ) {
          writer.noPrefixPrintln(",");
        }
        int cmdResult = executeLine(pszCmd);
        if (cmdResult) {
          break;
        }
        nextIndex += cmdLen + 1;
        if ( nextIndex >= scriptLen ) {
          break;
        } else {
          pszCmd = strtok(&pszScript[nextIndex], ";\r\n");
        }
      }
      writer.println();
      return cmdResult;
    }

    int executeLine(char *pszCmd,bool bVerbose=false) {
      int respCode = 0;
      //cout << __PRETTY_FUNCTION__ << " command: " << pszCmd << "." << endl;
      char *pszCmdName = strtok(pszCmd, ", ");
      if (!strcasecmp_P(pszCmdName, PSTR("SETUP"))) {
        respCode = processSetupCommand(bVerbose);
      } else if (!strcasecmp_P(pszCmdName, PSTR("RESET"))) {
        arduino::watchdog::resetRequested = true;
        beginResp() + F("Reset requested");
        endResp(0);
      } else if (!strcasecmp_P(pszCmdName, PSTR("SET"))) {
        respCode = processSetCommand(bVerbose);
      } else if (!strcasecmp_P(pszCmdName, PSTR("GET"))) {
        respCode = processGetCommand(bVerbose);
      } else if (!strcasecmp_P(pszCmdName, PSTR("INCLUDE"))) {
        respCode = processFilterCommand(bVerbose,true);
      } else if (!strcasecmp_P(pszCmdName, PSTR("EXCLUDE"))) {
        respCode = processFilterCommand(bVerbose,false);
      } else if (!strcasecmp_P(pszCmdName, PSTR("VERBOSE"))) {
        pszCmd = strtok(NULL, "\r\n");
        bVerbose = true;
        respCode = executeLine(pszCmd,bVerbose);
      } else {
        beginResp() + F("Expected {GET|INCLUDE|EXCLUDE|SET|SETUP|RESET|VERBOSE} but found: ") + pszCmdName;
        endResp(INVALID_CMD_ARGUMENT);
      }
      return respCode;
    }


  protected:

    int processSetupCommand(bool bVerbose) {
      int respCode = 0;
      const char *pszAction = strtok(NULL, ", \r\n");

      if (!strcasecmp_P(pszAction, PSTR("RUN"))) {
        return runSetup();
      }

      writer.println("{").increaseDepth();

      beginResp();

      if (!strcasecmp_P(pszAction, PSTR("SET"))) {  
        const char *pszField = strtok(NULL,", ");
        if ( !strcasecmp_P(pszField, PSTR("OUTPUT_FORMAT"))) {
          const char *pszFormat = strtok(NULL, ", \r\n");
          JsonFormat fmt = arduino::parseFormat(pszFormat);
          if ( fmt != JSON_FORMAT_INVALID ) {
            eeprom.setJsonFormat(fmt);
          } else {
            writer + F("Expected JSON_COMPACT|JSON_PRETTY but found: ") + pszFormat;
            respCode = INVALID_CMD_ARGUMENT;
          }
        } else {
          writer + F("Expected SET field {OUTPUT_FORMAT} but found: ") + pszField;
          respCode = INVALID_CMD_ARGUMENT;
        }
      } else if (!strcasecmp_P(pszAction, PSTR("ADD"))) {
        const char *pszCmd = strtok(NULL,"\r\n");
        respCode = eeprom.addCommand(pszCmd);
        if ( respCode != CMD_OK ) {
          writer + errorDesc(F("SETUP,ADD"),respCode);
        }
      } else if (!strcasecmp_P(pszAction, PSTR("REPLACE_AT"))) {
        const char* pszIndex = strtok(NULL, ",\r\n");
        const char* pszCmd = strtok(NULL, "\r\n");
        int index = atoi(pszIndex);
        respCode = eeprom.setCommandAt(index,pszCmd);
        if ( respCode != CMD_OK ) {
          writer + errorDesc(F("SETUP,REPLACE_AT"),respCode);
        }
      } else if (!strcasecmp_P(pszAction, PSTR("REMOVE"))) {
        const char* pszCmd = strtok(NULL, "\r\n");
        respCode = eeprom.removeCommand(pszCmd);
        if ( respCode != CMD_OK ) {
          writer + errorDesc(F("SETUP,REMOVE"),respCode);
        }
      } else if (!strcasecmp_P(pszAction, PSTR("REMOVE_AT"))) {
        const char* pszIndex = strtok(NULL, ",\r\n");
        int index = atoi(pszIndex);
        respCode = eeprom.removeCommandAt(index);
        if ( respCode != CMD_OK ) {
          writer + errorDesc(F("SETUP,REMOVE_AT"),respCode);
        }
      } else {
        writer + F("SETUP expected {RUN|SET|ADD|REPLACE_AT|REMOVE|REMOVE_AT} but found: '") + pszAction + "'.";
        respCode = INVALID_CMD_ARGUMENT;
      }
      endResp(respCode);
      
      writer.decreaseDepth().print("}");
      return respCode;
    }

    int processGetCommand(bool bVerbose) {
      int respCode = 0;
      const char *pszArg = strtok(NULL, ", ");

      if (pszArg==nullptr) {
        pszArg = "";
      }

      writer.println("{").increaseDepth();
      do {
        if (!strcasecmp_P(pszArg, PSTR("ENV"))) {
          writer.printKey("env");
          writer.noPrefixPrintln("{");
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
          //writer.printKey(F("eeprom"));
          //eeprom.print(1);
          writer.decreaseDepth();
          writer.println("},");
        } else if (!strcasecmp_P(pszArg, PSTR("SETUP")) || !strcasecmp_P(pszArg, PSTR("EEPROM"))) {
          writer.printKey(F("eeprom"));
          eeprom.noPrefixPrint(2);
          writer.noPrefixPrintln(",");
        } else if (!strcasecmp_P(pszArg, PSTR("OUTPUT_FORMAT"))) {
          writer.printlnStringObj(F("outputFormat"), formatAsString(arduino::jsonFormat), ",");
        } else if (!strcasecmp_P(pszArg, PSTR("SENSORS"))) {
          writer.printlnVectorObj(F("sensors"), sensors, ",",bVerbose);
        } else if (!strcasecmp_P(pszArg, PSTR("DEVICES"))) {
          writer.printlnVectorObj(F("devices"), devices, ",",bVerbose);
        } else if (!strcasecmp_P(pszArg, PSTR("TIME"))) {
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
          respCode = INVALID_CMD_ARGUMENT;
          break;
        }        
      } while ((pszArg=strtok(NULL, ", ")) != nullptr);
      if (!respCode) {
        beginResp();
        writer + "OK";
      }
      endResp(respCode);

      writer.decreaseDepth().print("}");
      return respCode;
    }

    int processFilterCommand(bool bVerbose,bool bInclude) {
      int respCode = 0;
      const char* pszArg = strtok(NULL, ", ");

      writer.println("{").increaseDepth();

      if (!strcasecmp_P(pszArg, PSTR("SENSORS"))) {
        const char* pszCommaDelimitedNames = strtok(NULL,"\r\n");
        Sensors filteredSensors;
        sensors.filterByNames(pszCommaDelimitedNames,filteredSensors,bInclude);
        writer.printlnVectorObj(F("sensors"), filteredSensors, ",",bVerbose);
      } else if (!strcasecmp_P(pszArg, PSTR("DEVICES"))) {
        const char* pszCommaDelimitedNames = strtok(NULL,"\r\n");
        Devices filteredDevices;
        devices.filterByNames(pszCommaDelimitedNames,filteredDevices,bInclude);
        writer.printlnVectorObj(F("devices"), filteredDevices, ",",bVerbose);
      } else {
        beginResp();
        writer + F("FILTER command expected {SENSORS|DEVICES} but found: '") + pszArg + "'.";
        respCode = INVALID_CMD_ARGUMENT;
      }
      if (!respCode) {
        beginResp();
        writer + "OK";
      }
      endResp(respCode);

      writer.decreaseDepth().print("}");
      return respCode;
    }
    
    int processSetCommand(bool bVerbose) {
      int respCode = 0;

      writer.println("{").increaseDepth();

      beginResp();

      const char *pszArg = strtok(NULL, ", ");

      if (!strcasecmp_P(pszArg, PSTR("OUTPUT_FORMAT"))) {

        const char *pszFormat = strtok(NULL, ", \r\n");
        JsonFormat fmt = arduino::parseFormat(pszFormat);
        if ( fmt != JSON_FORMAT_INVALID ) {
          arduino::jsonFormat = fmt;
        } else {
          writer + F("Expected JSON_COMPACT|JSON_PRETTY but found: ") + pszFormat;
          respCode = 301;
        }
      } else if (!strcasecmp_P(pszArg, PSTR("TIME_T"))) {
        
        const char *pszDateTime = strtok(NULL, ", ");
        time_t time = atol(pszDateTime);
        setTime(time);
        writer + F("TIME_T set to ") + time;

      } else if (!strcasecmp_P(pszArg, PSTR("TIME"))) {
        const char* pszYear = strtok(NULL, ", ");
        const char* pszMonth = strtok(NULL, ", ");
        const char* pszDay = strtok(NULL, ", ");
        const char* pszHour = strtok(NULL, ", ");
        const char* pszMinute = strtok(NULL, ", ");
        const char* pszSecond = strtok(NULL, ", ");
        if ( !pszYear || !pszMonth || !pszDay || !pszHour || !pszMinute || !pszSecond ) {
          writer + F("Expected date in YYYY,MM,DD,HH,mm,SS format.");
          respCode = INVALID_CMD_ARGUMENT;
        } else {
          int year = atoi(pszYear), month = atoi(pszMonth), day = atoi(pszDay),
              hour = atoi(pszHour), minute = atoi(pszMinute), second = atoi(pszSecond);
          setTime(hour, minute, second, day, month, year);
          writer + F("TIME set using YYYY,MM,DD,HH,mm,SS args: ");
          for( int i : {year,month,day,hour,minute,second} ) {
            writer + i + ",";
          }
        }
      } else if (!strcasecmp_P(pszArg, PSTR("DEVICE")) || !strcasecmp_P(pszArg, PSTR("SENSOR")) ) {
        const char *pszName = strtok(NULL, ",\r\n");
        const char *pszKey = strtok(NULL, ", \r\n");  // ex: "CAPABILITY/TOGGLE","MODE","NAME",etc...
        const char *pszVal = strtok(NULL, ", \r\n");
        stringstream ss;
        bool bUpdated = false;
        int filteredCnt = 0;
        if ( !strcasecmp_P(pszArg, PSTR("DEVICE")) ) {
          Devices filteredDevices;
          devices.filterByNames(pszName,filteredDevices);
          filteredCnt = filteredDevices.size();
          for (Device *pDevice : filteredDevices) {
            if ( pDevice->setAttribute(pszKey,pszVal,&ss) ) {
              bUpdated = true;
            }
          }
        } else {
          Sensors filteredSensors;
          sensors.filterByNames(pszName,filteredSensors);
          filteredCnt = filteredSensors.size();
          for (Sensor *pSensor : filteredSensors) {
            if ( pSensor->setAttribute(pszKey,pszVal,&ss) ) {
              bUpdated = true;
            }
          }
        }
        if (bUpdated) {
          writer + ss.str().c_str();
        } else if (filteredCnt==0) {
          writer + F("No matches for ") + pszArg + F(" with name matching '") + pszName + F("'");
          respCode = NO_NAME_MATCHES;
        } else {
          writer + F("No ") + pszArg + F(" with key matching '") + pszKey + F("'");
          respCode = NO_NAME_MATCHES;
        }
      } else {
        writer + F("Expected TIME_T|TIME|DEVICE|SENSOR|SETUP but found: ") + pszArg;
        respCode = INVALID_CMD_ARGUMENT;
      }
      endResp(respCode);

      writer.decreaseDepth().print("}");
      return respCode;
    }    

  };

}
#endif //ARDUINO_SOLAR_SKETCH_COMMANDPROCESSOR_H
