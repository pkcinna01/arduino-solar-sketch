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
      if (!strcasecmp_P(pszCmdName, PSTR("setup")) || !strcasecmp_P(pszCmdName, PSTR("eeprom"))) {
        respCode = processSetupCommand();
      } else if (!strcasecmp_P(pszCmdName, PSTR("RESET"))) {
        arduino::watchdog::resetRequested = true;
        beginResp() + F("Reset requested");
        endResp(0);
      } else if (!strcasecmp_P(pszCmdName, PSTR("SET"))) {
        respCode = processSetCommand();
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
      } else if (!strcasecmp_P(pszCmdName, PSTR("PAUSE")) || !strcasecmp_P(pszCmdName, PSTR("RESUME")) ) {
        bool bPause = !strcasecmp_P(pszCmdName, PSTR("PAUSE"));
        if ( !bPause ) {
          Constraints::pauseEndTimer().expire();
        } else {
          const char* pszSeconds = strtok(NULL, "\r\n");
          if ( pszSeconds == nullptr ) {
            Constraints::pauseEndTimer().setMaxDurationMs(0).reset(); // using zero duration as flag for disabled to save space
          } else {
            unsigned long seconds = atol(pszSeconds);
            Constraints::pauseEndTimer().setMaxDurationAsSeconds(seconds).reset();
          }
        }
        writer.println("{").increaseDepth();
        beginResp() + F("Constraint processing ") + (Constraints::isPaused()?F("paused"):F("resumed"));
        endResp(0);
        writer.decreaseDepth().print("}");
      } else {
        beginResp() + F("Expected {get|include|exclude|set|setup|eeprom|reset|verbose|pause|resume} but found: ") + (pszCmdName?pszCmdName:"");
        endResp(INVALID_ARGUMENT);
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

      const char* pszNamePattern = strtok(NULL,"\r\n");
      AttributeContainerVector<AttributeContainer*> filteredVec;

      if (!strcasecmp_P(pszArg, PSTR("SENSORS"))) {
        sensors.findByTitleLike(pszNamePattern,filteredVec,bInclude);
      } else if (!strcasecmp_P(pszArg, PSTR("DEVICES"))) {
        devices.findByTitleLike(pszNamePattern,filteredVec,bInclude);
      } else if (!strcasecmp_P(pszArg, PSTR("CONSTRAINTS"))) {
        Constraints(Constraint::all()).findByTitleLike(pszNamePattern,filteredVec,bInclude);
      } else if (!strcasecmp_P(pszArg, PSTR("CAPABILITIES"))) {
        Capabilities(Capability::all()).findByTitleLike(pszNamePattern,filteredVec,bInclude);
      } else {
        beginResp();
        writer + F("FILTER command expected {SENSORS|DEVICES|CONSTRAINTS|CAPABILITIES} but found: '") + pszArg + "'.";
        respCode = INVALID_ARGUMENT;
      }
      if (!respCode) {
        String key(pszArg);
        key.toLowerCase();
        writer.printlnVectorObj(key, filteredVec, ",", bVerbose);
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
        if (!strcasecmp_P(pszArg, PSTR("env"))) {
          writer.printKey(F("env"));
          writer.noPrefixPrintln("{");
          writer.increaseDepth();
          writer.printlnStringObj(F("version"), VERSION, ",")
              .printlnNumberObj(F("buildNumber"), BUILD_NUMBER, ",")
              .printlnStringObj(F("buildDate"), BUILD_DATE, ",")
              .printlnStringObj(F("vcc"), readVcc(), ",");
          writer.beginStringObj(F("time"));
          time_t t = now();
          writer + year(t) + "-" + month(t) + "-" + day(t) + " " + hour(t) + ":" + minute(t) + ":" + second(t);
          writer.endStringObj();
          writer.noPrefixPrintln(",");
          writer.printlnBoolObj("timeSet", automation::isTimeValid());
          writer.decreaseDepth();
          writer.println("},");
        } else if (!strcasecmp_P(pszArg, PSTR("eeprom")) || !strcasecmp_P(pszArg, PSTR("setup"))) {
          writer.printKey(F("eeprom"));
          eeprom.noPrefixPrint(writer);
          writer.noPrefixPrintln(",");
        } else if (!strcasecmp_P(pszArg, PSTR("isPaused"))) {
          writer.printlnBoolObj(F("isPaused"), Constraints::isPaused(), ",");
        } else if (!strcasecmp_P(pszArg, PSTR("jsonFormat"))) {
          writer.printlnStringObj(F("jsonFormat"), formatAsString(jsonFormat).c_str(), ",");
        } else if (!strcasecmp_P(pszArg, PSTR("SENSOR")) 
                || !strcasecmp_P(pszArg, PSTR("DEVICE")) 
                || !strcasecmp_P(pszArg, PSTR("CAPABILITY")) 
                || !strcasecmp_P(pszArg, PSTR("CONSTRAINT"))) {
          string automationType(pszArg);
          std::transform(automationType.begin(), automationType.end(), automationType.begin(), ::tolower);
          AttributeContainerVector<AttributeContainer*> resultVec;
          std::vector<unsigned long> ids;
          const char* pszId;
          while ( (pszId=strtok(NULL, ",\r\n")) != NULL ) {
            ids.push_back(atol(pszId));
          }
          if ( ids.empty() ) {
            beginResp();
            writer + F("ID required for GET of ") + pszArg + ".";
            respCode = INVALID_ARGUMENT;
          } else {
            if( !strcasecmp_P(pszArg, PSTR("SENSOR")) ) {
              sensors.findByIds(ids,resultVec);
            } else if( !strcasecmp_P(pszArg, PSTR("CONSTRAINT")) ) {
              Constraints(Constraint::all()).findByIds(ids,resultVec);
            } else if( !strcasecmp_P(pszArg, PSTR("CAPABILITY")) ) {
              Capabilities(Capability::all()).findByIds(ids,resultVec);
            } else if( !strcasecmp_P(pszArg, PSTR("DEVICE")) ) {
              devices.findByIds(ids,resultVec);
            }
            if ( resultVec.size() == ids.size() ) {
              //singletonVec[0]->printlnObj( writer, automationType.c_str(), ",", bVerbose);
              writer.printlnVectorObj(automationType.c_str(), resultVec, ",",bVerbose);
            } else {
              beginResp();
              writer + F("Expected ") + ids.size() + " " + pszArg + F(" result(s) but found ") + resultVec.size();
              respCode = resultVec.size() < ids.size() ? NOT_FOUND : CMD_ERROR;
            }
          }
        } else if (!strcasecmp_P(pszArg, PSTR("SENSORS"))) {
          writer.printlnVectorObj(F("sensors"), sensors, ",",bVerbose);
        } else if (!strcasecmp_P(pszArg, PSTR("DEVICES"))) {
          writer.printlnVectorObj(F("devices"), devices, ",",bVerbose);
        } else if (!strcasecmp_P(pszArg, PSTR("CONSTRAINTS"))) {
          writer.printlnVectorObj(F("constraints"), Constraint::all(), ",",bVerbose);
        } else if (!strcasecmp_P(pszArg, PSTR("CAPABILITIES"))) {
          writer.printlnVectorObj(F("capabilities"), Capability::all(), ",",bVerbose);
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
          writer.printlnBoolObj("timeSet", automation::isTimeValid());
          writer.decreaseDepth();
          writer.println("},");
        } else {
          beginResp();
          writer + F("get command expected {sensors|devices|jsonFormat|time|env|setup|eeprom} but found: '") + pszArg + "'.";
          respCode = INVALID_ARGUMENT;
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

    int processSetCommand() {
      int respCode = 0;

      writer.println("{").increaseDepth();

      beginResp();

      const char *pszArg = strtok(NULL, ", ");

      if (!strcasecmp_P(pszArg, PSTR("jsonFormat"))) {

        const char *pszFormat = strtok(NULL, ", \r\n");
        JsonFormat fmt = parseFormat(pszFormat);
        if ( fmt != JsonFormat::INVALID ) {
          jsonFormat = fmt;
        } else {
          writer + F("Expected COMPACT|PRETTY but found: ") + pszFormat;
          respCode = INVALID_ARGUMENT;
        }
      } else if (!strcasecmp_P(pszArg, PSTR("TIME"))) {
        const char* pszYear = strtok(NULL, ", ");
        const char* pszMonth = strtok(NULL, ", ");
        const char* pszDay = strtok(NULL, ", ");
        const char* pszHour = strtok(NULL, ", ");
        const char* pszMinute = strtok(NULL, ", ");
        const char* pszSecond = strtok(NULL, ", ");
        if ( !pszYear || !pszMonth || !pszDay || !pszHour || !pszMinute || !pszSecond ) {
          writer + F("Expected date in YYYY,MM,DD,HH,mm,SS format.");
          respCode = INVALID_ARGUMENT;
        } else {
          int year = atoi(pszYear), month = atoi(pszMonth), day = atoi(pszDay),
              hour = atoi(pszHour), minute = atoi(pszMinute), second = atoi(pszSecond);
          setTime(hour, minute, second, day, month, year);
          writer + F("TIME set using YYYY,MM,DD,HH,mm,SS args: ");
          for( int i : {year,month,day,hour,minute,second} ) {
            writer + i + ",";
          }
        }
      } else if (!strcasecmp_P(pszArg, PSTR("DEVICES")) 
              || !strcasecmp_P(pszArg, PSTR("SENSORS")) 
              || !strcasecmp_P(pszArg, PSTR("CAPABILITIES"))
              || !strcasecmp_P(pszArg, PSTR("CONSTRAINTS")) ) {
        const char *pszName = strtok(NULL, ",\r\n");
        const char *pszKey = strtok(NULL, ", \r\n");  
        const char *pszVal = strtok(NULL, ", \r\n");
        SetCode rtn = SetCode::Ignored;
        AttributeContainerVector<AttributeContainer*> filteredVec;
        stringstream ss;
        if ( !strcasecmp_P(pszArg, PSTR("DEVICES")) ) {
          devices.findByTitleLike(pszName,filteredVec);
        } else if ( !strcasecmp_P(pszArg, PSTR("CONSTRAINTS")) ) {
          Constraints(Constraint::all()).findByTitleLike(pszName,filteredVec);
        } else if ( !strcasecmp_P(pszArg, PSTR("CAPABILITIES")) ) {
          Capabilities(Capability::all()).findByTitleLike(pszName,filteredVec);
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
          respCode = NOT_FOUND;
        } else {
          writer + pszArg + F(" set '") + pszKey + F("' failed.");
          string reason = ss.str().c_str();
          if ( !reason.empty() ) {
            writer + " " + reason;
          }
          respCode = CMD_ERROR;
        }
      } else if (!strcasecmp_P(pszArg, PSTR("DEVICE")) 
              || !strcasecmp_P(pszArg, PSTR("SENSOR")) 
              || !strcasecmp_P(pszArg, PSTR("CAPABILITY"))
              || !strcasecmp_P(pszArg, PSTR("CONSTRAINT")) ) {
        const char *pszId = strtok(NULL, ",\r\n");
        const char *pszKey = strtok(NULL, ", \r\n");  // ex: "CAPABILITY/TOGGLE","MODE","NAME",etc...
        const char *pszVal = strtok(NULL, ", \r\n");
        stringstream ss;
        SetCode rtn = SetCode::Ignored;
        unsigned long id = atol(pszId);
        AttributeContainerVector<AttributeContainer*> filteredVec;
        if ( !strcasecmp_P(pszArg, PSTR("DEVICE")) ) {
          devices.findById(id,filteredVec);
        } else if ( !strcasecmp_P(pszArg, PSTR("CAPABILITY")) ) {
          Capabilities(Capability::all()).findById(id,filteredVec);
        } else if ( !strcasecmp_P(pszArg, PSTR("CONSTRAINT")) ) {
          Constraints(Constraint::all()).findById(id,filteredVec);
        } else {
          sensors.findById(id,filteredVec);
        }
        for (AttributeContainer *pAttrContainer : filteredVec) {
          SetCode code = pAttrContainer->setAttribute(pszKey,pszVal,&ss);
          if ( code != SetCode::Ignored && rtn != SetCode::Error ) {
            rtn = code; // continue if errors but keep 'rtn' as error for client
          }
        }
        if (rtn==SetCode::OK) {
          writer + ss.str().c_str();
        } else if (filteredVec.empty()) {
          writer + F("No matches for ") + pszArg + F(" with ID = '") + pszId + F("'");
          respCode = NOT_FOUND;
        } else {
          writer + pszArg + F(" set '") + pszKey + F("' failed.");
          string reason = ss.str().c_str();
          if ( !reason.empty() ) {
            writer + " " + reason;
          }
          respCode = CMD_ERROR;
        }
      } else {
        writer + F("Expected TIME|DEVICE|SENSOR|CONSTRAINT|DEVICES|SENSORS|CONSTRAINTS|jsonFormat but found: ") + pszArg;
        respCode = INVALID_ARGUMENT;
      }
      endResp(respCode);

      writer.decreaseDepth().print("}");
      return respCode;
    }    


    ////////////////////
    // SETUP (EEPROM) //
    ////////////////////

    int processSetupCommand() {
      int respCode = 0;
      const char *pszAction = strtok(NULL, ", \r\n");

      if (!strcasecmp_P(pszAction, PSTR("run"))) {
        return runSetup();
      }

      writer.println("{").increaseDepth();

      beginResp();

      if (!strcasecmp_P(pszAction, PSTR("set"))) {  
        const char *pszField = strtok(NULL,", ");
        if ( !strcasecmp_P(pszField, PSTR("deviceName"))) {
          const char *pszDeviceName = strtok(NULL, "\r\n");
          eeprom.setDeviceName(pszDeviceName);
          String str;
          writer + F("Device name set to: '") + eeprom.getDeviceName(str) + F("'");
        } else if ( !strcasecmp_P(pszField, PSTR("deviceId"))) {
          const char *pszDeviceId = strtok(NULL, ", \r\n");
          eeprom.setDeviceId(atol(pszDeviceId));
          writer + F("Device ID set to: '") + eeprom.getDeviceId() + F("'");
        } else if ( !strcasecmp_P(pszField, PSTR("jsonFormat"))) {
          const char *pszFormat = strtok(NULL, ", \r\n");
          JsonFormat fmt = parseFormat(pszFormat);
          if ( fmt != JsonFormat::INVALID ) {
            eeprom.setJsonFormat(fmt);
          } else {
            writer + F("Expected COMPACT|PRETTY but found: ") + pszFormat;
            respCode = INVALID_ARGUMENT;
          }
        } else if ( !strcasecmp_P(pszField, PSTR("serialSpeed"))) {
          const char *pszSpeed = strtok(NULL, ", \r\n");
          unsigned long speed = atol(pszSpeed);
          if ( automation::algorithm::indexOf(speed,{{9600, 14400, 19200, 28800, 38400, 57600, 115200}}) > 0 ) {
            eeprom.setSerialSpeed(speed);
            gLastInfoMsg = F("Serial communication changes require a RESET.");
          } else {
            writer + F("Unsupported serial speed: ") + pszSpeed;
            respCode = INVALID_ARGUMENT;
          }
        } else if ( !strcasecmp_P(pszField, PSTR("serialConfig"))) {
          const char *pszConfig = strtok(NULL, ", \r\n");
          std::map<string,unsigned int> validConfigs = { {"8N1",SERIAL_8N1}, {"8E1",SERIAL_8E1}, {"8O1",SERIAL_8O1} };
          auto configIt = validConfigs.find(pszConfig);
          if ( configIt != validConfigs.end() ) {
            eeprom.setSerialConfig(configIt->second);
            gLastInfoMsg = F("Serial communication changes require a RESET.");
          } else {
            writer + F("Expected 8N1|8E1|8O1 but found: ") + pszConfig;
            respCode = INVALID_ARGUMENT;
          }
        } else {
          writer + F("Expected SET field {deviceName|jsonFormat|serialSpeed|serialConfig} but found: ") + pszField;
          respCode = INVALID_ARGUMENT;
        }
      } else if (!strcasecmp_P(pszAction, PSTR("add"))) {
        const char *pszCmd = strtok(NULL,"\r\n");
        respCode = eeprom.addCommand(pszCmd);
        if ( respCode != CMD_OK ) {
          writer + errorDesc(F("setup,add"),respCode);
        }
      } else if (!strcasecmp_P(pszAction, PSTR("insertAt"))) {
        const char* pszIndex = strtok(NULL, ",\r\n");
        const char* pszCmd = strtok(NULL, "\r\n");
        int index = atoi(pszIndex);
        respCode = eeprom.insertCommandAt(index,pszCmd);
        if ( respCode != CMD_OK ) {
          writer + errorDesc(F("setup,insertAt"),respCode);
        }
      } else if (!strcasecmp_P(pszAction, PSTR("replaceAt"))) {
        const char* pszIndex = strtok(NULL, ",\r\n");
        const char* pszCmd = strtok(NULL, "\r\n");
        int index = atoi(pszIndex);
        respCode = eeprom.setCommandAt(index,pszCmd);
        if ( respCode != CMD_OK ) {
          writer + errorDesc(F("setup,replaceAt"),respCode);
        }
      } else if (!strcasecmp_P(pszAction, PSTR("replace")) || !strcasecmp_P(pszAction, PSTR("replaceOrAdd"))) {
        const char* pszDelimiter = strtok(NULL, ",\r\n");
        const char* pszSearchPattern = strtok(NULL, pszDelimiter);
        const char* pszCmd = strtok(NULL, "\r\n");
        respCode = eeprom.replaceCommand(pszSearchPattern,pszCmd,strcasecmp_P(pszAction, PSTR("replace")));
        if ( respCode != CMD_OK ) {
          writer + errorDesc(F("setup,replace"),respCode);
        }
      } else if (!strcasecmp_P(pszAction, PSTR("remove"))) {
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
      } else if (!strcasecmp_P(pszAction, PSTR("removeAll"))) {
        const char* pszCmd = strtok(NULL, "\r\n");
        int removedCnt = eeprom.removeCommand(pszCmd,/*bRemoveAllMatches=*/true);
        if ( removedCnt < 0 ) {
          respCode = removedCnt;
          writer + errorDesc(F("setup,removeAll"),respCode);
        } else {
          writer + F("Removed ") + removedCnt + F(" entries matching: '") + pszCmd + F("'");
        }
      } else if (!strcasecmp_P(pszAction, PSTR("removeAt"))) {
        const char* pszIndex = strtok(NULL, ",\r\n");
        int index = atoi(pszIndex);
        respCode = eeprom.removeCommandAt(index);
        if ( respCode != CMD_OK ) {
          writer + errorDesc(F("setup,removeAt"),respCode);
        }
      } else {
        writer + F("setup expected {run|set|add|insertAt|replace|replaceOrAdd|replaceAt|remove|removeAll|removeAt} but found: '") + pszAction + "'.";
        respCode = INVALID_ARGUMENT;
      }
      endResp(respCode);
      
      writer.decreaseDepth().print("}");
      return respCode;
    }



  };

}
#endif //ARDUINO_SOLAR_SKETCH_COMMANDPROCESSOR_H
