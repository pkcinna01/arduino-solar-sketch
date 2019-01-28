#ifndef ARDUINO_SOLAR_SKETCH_COMMANDPROCESSOR_H
#define ARDUINO_SOLAR_SKETCH_COMMANDPROCESSOR_H


#include "Arduino.h"
#include "../automation/json/JsonStreamWriter.h"
#include "../automation/json/json.h"
#include "Eeprom.h"

#include "../automation/capability/Capability.h"
#include "watchdog.h"

#include <vector>
#include <map>
#include <string>

using namespace std;
using namespace automation::json;

namespace arduino {

  class CommandProcessor {

  public:

    JsonStreamWriter &writer;
    Sensors &sensors;
    Devices &devices;

    static void setup(JsonStreamWriter& writer, Sensors &sensors, Devices &devices) {

      // run script lines from EEPROM
      CommandProcessor cmdProcessor(writer,sensors,devices);
      cmdProcessor.runSetup();
    }

    CommandProcessor(JsonStreamWriter &writer, Sensors &sensors, Devices &devices)
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

    JsonStreamWriter &beginResp() {
      writer.beginStringObj(F("respMsg"));
      return writer;
    }

    JsonStreamWriter &endResp(int respCode) {
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
        beginResp() + F("Expected {GET|INCLUDE|EXCLUDE|SET|SETUP|RESET|VERBOSE} but found: ") + (pszCmdName?pszCmdName:"");
        endResp(INVALID_CMD_ARGUMENT);
      }
      return respCode;
    }


  protected:

    ////////////
    // FILTER //
    ////////////

    int processFilterCommand(bool bVerbose,bool bInclude) {
      int respCode = 0;
      const char* pszArg = strtok(NULL, ", ");

      writer.println("{").increaseDepth();

      if (!strcasecmp_P(pszArg, PSTR("SENSORS"))) {
        const char* pszNamePattern = strtok(NULL,"\r\n");
        Sensors filteredSensors;
        sensors.findByTitleLike(pszNamePattern,filteredSensors,bInclude);
        writer.printlnVectorObj(F("sensors"), filteredSensors, ",",bVerbose);
      } else if (!strcasecmp_P(pszArg, PSTR("DEVICES"))) {
        const char* pszNamePattern = strtok(NULL,"\r\n");
        Devices filteredDevices;
        devices.findByTitleLike(pszNamePattern,filteredDevices,bInclude);
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
    
    /////////
    // GET //
    /////////

    int processGetCommand(bool bVerbose) {
      int respCode = 0;
      const char *pszArg = strtok(NULL, ", ");

      if (pszArg==nullptr) {
        pszArg = "";
      }

      writer.println("{").increaseDepth();
      do {
        if (!strcasecmp_P(pszArg, PSTR("ENV"))) {
          writer.printKey(F("env"));
          writer.noPrefixPrintln("{");
          writer.increaseDepth();
          writer.printlnStringObj(F("release"), VERSION, ",")
              .printlnNumberObj(F("buildNumber"), BUILD_NUMBER, ",")
              .printlnStringObj(F("buildDate"), BUILD_DATE, ",")
              .printlnStringObj(F("Vcc"), readVcc(), ",");
          writer.beginStringObj(F("time"));
          time_t t = now();
          writer + year(t) + "-" + month(t) + "-" + day(t) + " " + hour(t) + ":" + minute(t) + ":" + second(t);
          writer.endStringObj();
          writer.noPrefixPrintln(",");
          writer.printlnStringObj("timeSet", timeStatus() == timeSet ? "YES" : "NO");
          writer.decreaseDepth();
          writer.println("},");
        } else if (!strcasecmp_P(pszArg, PSTR("SETUP")) || !strcasecmp_P(pszArg, PSTR("EEPROM"))) {
          writer.printKey(F("eeprom"));
          eeprom.noPrefixPrint(writer);
          writer.noPrefixPrintln(",");
        } else if (!strcasecmp_P(pszArg, PSTR("OUTPUT_FORMAT"))) {
          writer.printlnStringObj(F("outputFormat"), formatAsString(jsonFormat).c_str(), ",");
        } else if (!strcasecmp_P(pszArg, PSTR("SENSOR")) || !strcasecmp_P(pszArg, PSTR("DEVICE")) || !strcasecmp_P(pszArg, PSTR("CONSTRAINT"))) {
          const char* pszId = strtok(NULL, ",\r\n");
          if ( !pszId ) {
              beginResp();
              writer + F("ID required for GET of single ") + pszArg + ".";
              respCode = INVALID_CMD_ARGUMENT;
          } else {
            long id = atol(pszId);
            string automationType(pszArg);
            std::transform(automationType.begin(), automationType.end(), automationType.begin(), ::tolower);
            AttributeContainerVector<AttributeContainer*> singletonVec;
            if( !strcasecmp_P(pszArg, PSTR("SENSOR")) ) {
              sensors.findById(id,singletonVec);
            } else if( !strcasecmp_P(pszArg, PSTR("CONSTRAINT")) ) {
              Constraints(Constraint::all()).findById(id,singletonVec);
            } else if( !strcasecmp_P(pszArg, PSTR("DEVICE")) ) {
              devices.findById(id,singletonVec);
            }
            if ( singletonVec.size() == 1 ) {
              writer.printlnVectorObj( automationType.c_str(), singletonVec, ",",bVerbose);
            } else {
              beginResp();
              if ( atol(pszId) > NumericIdentifierMax ) {
                writer + F("ID '") + pszId + F("' exceeded maximum '") + NumericIdentifierMax + F("'");
              } else {
                writer + F("Expected one ") + pszArg + F(" for ID '") + pszId + F("' but found ") + singletonVec.size();
              }
              respCode = NO_NAME_MATCHES;
            }
          }
        } else if (!strcasecmp_P(pszArg, PSTR("CONSTRAINTS"))) {
          writer.printlnVectorObj(F("constraints"), Constraint::all(), ",",bVerbose);
        } else if (!strcasecmp_P(pszArg, PSTR("SENSORS"))) {
          writer.printlnVectorObj(F("sensors"), sensors, ",",bVerbose);
        } else if (!strcasecmp_P(pszArg, PSTR("DEVICES"))) {
          writer.printlnVectorObj(F("devices"), devices, ",",bVerbose);
        } else if (!strcasecmp_P(pszArg, PSTR("TIME"))) {
          time_t t = now();
          writer.printKey("time");
          writer.noPrefixPrintln("{");
          writer.increaseDepth();
          writer.printlnNumberObj("year", year(t), ",");
          writer.printlnNumberObj("month", month(t), ",");
          writer.printlnNumberObj("day", day(t), ",");
          writer.printlnNumberObj("hour", hour(t), ",");
          writer.printlnNumberObj("minute", minute(t), ",");
          writer.printlnNumberObj("second", second(t), ",");
          writer.printlnStringObj("timeSet", timeStatus() == timeSet ? "YES" : "NO");
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
    

    /////////
    // SET //
    /////////

    int processSetCommand(bool bVerbose) {
      int respCode = 0;

      writer.println("{").increaseDepth();

      beginResp();

      const char *pszArg = strtok(NULL, ", ");

      if (!strcasecmp_P(pszArg, PSTR("OUTPUT_FORMAT"))) {

        const char *pszFormat = strtok(NULL, ", \r\n");
        JsonFormat fmt = parseFormat(pszFormat);
        if ( fmt != JSON_FORMAT_INVALID ) {
          jsonFormat = fmt;
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
      } else if (!strcasecmp_P(pszArg, PSTR("DEVICES")) || !strcasecmp_P(pszArg, PSTR("SENSORS")) || !strcasecmp_P(pszArg, PSTR("CONSTRAINTS")) ) {
        const char *pszName = strtok(NULL, ",\r\n");
        const char *pszKey = strtok(NULL, ", \r\n");  // ex: "CAPABILITY/TOGGLE","MODE","NAME",etc...
        const char *pszVal = strtok(NULL, ", \r\n");
        SetCode rtn = SetCode::Ignored;
        AttributeContainerVector<AttributeContainer*> filteredVec;
        stringstream ss;
        if ( !strcasecmp_P(pszArg, PSTR("DEVICES")) ) {
          devices.findByTitleLike(pszName,filteredVec);
        } else if ( !strcasecmp_P(pszArg, PSTR("CONSTRAINTS")) ) {
          Constraints(Constraint::all()).findByTitleLike(pszName,filteredVec);
        } else {
          sensors.findByTitleLike(pszName,filteredVec);
        }
        for (AttributeContainer *pAttrContainer : filteredVec) {
          SetCode code = pAttrContainer->setAttribute(pszKey,pszVal,&ss);
          if ( code != SetCode::Ignored && rtn != SetCode::Error ) {
            rtn = code;
          }
        }
        if (rtn==SetCode::OK) {
          writer + ss.str();
        } else if (filteredVec.empty()) {
          writer + F("No matches for ") + pszArg + F(" with name matching '") + pszName + F("'");
          respCode = NO_NAME_MATCHES;
        } else {
          writer + pszArg + F(" set '") + pszKey + F("' failed.");
          string reason = ss.str().c_str();
          if ( !reason.empty() ) {
            writer + " " + reason;
          }
          respCode = NO_NAME_MATCHES;
        }
      } else if (!strcasecmp_P(pszArg, PSTR("DEVICE")) || !strcasecmp_P(pszArg, PSTR("SENSOR")) || !strcasecmp_P(pszArg, PSTR("CONSTRAINT")) ) {
        const char *pszId = strtok(NULL, ",\r\n");
        const char *pszKey = strtok(NULL, ", \r\n");  // ex: "CAPABILITY/TOGGLE","MODE","NAME",etc...
        const char *pszVal = strtok(NULL, ", \r\n");
        stringstream ss;
        SetCode rtn = SetCode::Ignored;
        unsigned long id = atol(pszId);
        AttributeContainerVector<AttributeContainer*> filteredVec;
        if ( !strcasecmp_P(pszArg, PSTR("DEVICE")) ) {
          devices.findById(id,filteredVec);
        } else if ( !strcasecmp_P(pszArg, PSTR("CONSTRAINT")) ) {
          Constraints(Constraint::all()).findById(id,filteredVec);
        } else {
          sensors.findById(id,filteredVec);
        }
        for (AttributeContainer *pAttrContainer : filteredVec) {
          SetCode code = pAttrContainer->setAttribute(pszKey,pszVal,&ss);
          if ( code != SetCode::Ignored && rtn != SetCode::Error ) {
            rtn = code;
          }
        }
        if (rtn==SetCode::OK) {
          writer + ss.str().c_str();
        } else if (filteredVec.empty()) {
          writer + F("No matches for ") + pszArg + F(" with ID = '") + pszId + F("'");
          respCode = NO_NAME_MATCHES;
        } else {
          writer + pszArg + F(" set '") + pszKey + F("' failed.");
          string reason = ss.str().c_str();
          if ( !reason.empty() ) {
            writer + " " + reason;
          }
          respCode = NO_NAME_MATCHES;
        }
      } else {
        writer + F("Expected TIME_T|TIME|DEVICE|SENSOR|CONSTRAINT|DEVICES|SENSORS|CONSTRAINTS|SETUP but found: ") + pszArg;
        respCode = INVALID_CMD_ARGUMENT;
      }
      endResp(respCode);

      writer.decreaseDepth().print("}");
      return respCode;
    }    


    ///////////
    // SETUP //
    ///////////

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
          JsonFormat fmt = parseFormat(pszFormat);
          if ( fmt != JSON_FORMAT_INVALID ) {
            eeprom.setJsonFormat(fmt);
          } else {
            writer + F("Expected JSON_COMPACT|JSON_PRETTY but found: ") + pszFormat;
            respCode = INVALID_CMD_ARGUMENT;
          }
        } else if ( !strcasecmp_P(pszField, PSTR("SERIAL_SPEED"))) {
          const char *pszSpeed = strtok(NULL, ", \r\n");
          unsigned long speed = atol(pszSpeed);
          if ( automation::algorithm::indexOf(speed,{{9600, 14400, 19200, 28800, 38400, 57600, 115200}}) > 0 ) {
            eeprom.setSerialSpeed(speed);
            gLastInfoMsg = F("Serial communication changes require a RESET.");
          } else {
            writer + F("Unsupported serial speed: ") + pszSpeed;
            respCode = INVALID_CMD_ARGUMENT;
          }
        } else if ( !strcasecmp_P(pszField, PSTR("SERIAL_CONFIG"))) {
          const char *pszConfig = strtok(NULL, ", \r\n");
          std::map<string,unsigned int> validConfigs = { {"8N1",SERIAL_8N1}, {"8E1",SERIAL_8E1}, {"8O1",SERIAL_8O1} };
          auto configIt = validConfigs.find(pszConfig);
          if ( configIt != validConfigs.end() ) {
            eeprom.setSerialConfig(configIt->second);
            gLastInfoMsg = F("Serial communication changes require a RESET.");
          } else {
            writer + F("Expected 8N1|8E1|8O1 but found: ") + pszConfig;
            respCode = INVALID_CMD_ARGUMENT;
          }
        } else {
          writer + F("Expected SET field {OUTPUT_FORMAT|SERIAL_SPEED|SERIAL_CONFIG} but found: ") + pszField;
          respCode = INVALID_CMD_ARGUMENT;
        }
      } else if (!strcasecmp_P(pszAction, PSTR("ADD"))) {
        const char *pszCmd = strtok(NULL,"\r\n");
        respCode = eeprom.addCommand(pszCmd);
        if ( respCode != CMD_OK ) {
          writer + errorDesc(F("SETUP,ADD"),respCode);
        }
      } else if (!strcasecmp_P(pszAction, PSTR("INSERT_AT"))) {
        const char* pszIndex = strtok(NULL, ",\r\n");
        const char* pszCmd = strtok(NULL, "\r\n");
        int index = atoi(pszIndex);
        respCode = eeprom.insertCommandAt(index,pszCmd);
        if ( respCode != CMD_OK ) {
          writer + errorDesc(F("SETUP,INSERT_AT"),respCode);
        }
      } else if (!strcasecmp_P(pszAction, PSTR("REPLACE_AT"))) {
        const char* pszIndex = strtok(NULL, ",\r\n");
        const char* pszCmd = strtok(NULL, "\r\n");
        int index = atoi(pszIndex);
        respCode = eeprom.setCommandAt(index,pszCmd);
        if ( respCode != CMD_OK ) {
          writer + errorDesc(F("SETUP,REPLACE_AT"),respCode);
        }
      } else if (!strcasecmp_P(pszAction, PSTR("REPLACE")) || !strcasecmp_P(pszAction, PSTR("REPLACE_OR_ADD"))) {
        const char* pszDelimiter = strtok(NULL, ",\r\n");
        const char* pszSearchPattern = strtok(NULL, pszDelimiter);
        const char* pszCmd = strtok(NULL, "\r\n");
        respCode = eeprom.replaceCommand(pszSearchPattern,pszCmd,strcasecmp_P(pszAction, PSTR("REPLACE")));
        if ( respCode != CMD_OK ) {
          writer + errorDesc(F("SETUP,REPLACE"),respCode);
        }
      } else if (!strcasecmp_P(pszAction, PSTR("REMOVE"))) {
        const char* pszCmd = strtok(NULL, "\r\n");
        respCode = eeprom.removeCommand(pszCmd);
        if ( respCode == 0 ) {
          writer + F("Nothing removed for: '") + pszCmd + F("'");
        } else if ( respCode < 0 ) {
          writer + errorDesc(F("SETUP,REMOVE"),respCode);
        } else if ( respCode > 1 ) {
          writer + F("Unexpected state... multiple items removed (count=") + respCode + F(").");
          respCode = CMD_ERROR;
        } else {
          respCode = CMD_OK;
        }
      } else if (!strcasecmp_P(pszAction, PSTR("REMOVE_ALL"))) {
        const char* pszCmd = strtok(NULL, "\r\n");
        int removedCnt = eeprom.removeCommand(pszCmd,/*bRemoveAllMatches=*/true);
        if ( removedCnt < 0 ) {
          respCode = removedCnt;
          writer + errorDesc(F("SETUP,REMOVE_ALL"),respCode);
        } else {
          writer + F("Removed ") + removedCnt + F(" entries matching: '") + pszCmd + F("'");
        }
      } else if (!strcasecmp_P(pszAction, PSTR("REMOVE_AT"))) {
        const char* pszIndex = strtok(NULL, ",\r\n");
        int index = atoi(pszIndex);
        respCode = eeprom.removeCommandAt(index);
        if ( respCode != CMD_OK ) {
          writer + errorDesc(F("SETUP,REMOVE_AT"),respCode);
        }
      } else {
        writer + F("SETUP expected {RUN|SET|ADD|INSERT_AT|REPLACE|REPLACE_OR_ADD|REPLACE_AT|REMOVE|REMOVE_ALL|REMOVE_AT} but found: '") + pszAction + "'.";
        respCode = INVALID_CMD_ARGUMENT;
      }
      endResp(respCode);
      
      writer.decreaseDepth().print("}");
      return respCode;
    }



  };

}
#endif //ARDUINO_SOLAR_SKETCH_COMMANDPROCESSOR_H
