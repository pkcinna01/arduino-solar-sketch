#ifndef ARDUINO_SOLAR_SKETCH_EEPROM_H
#define ARDUINO_SOLAR_SKETCH_EEPROM_H

#include "Arduino.h"
#include "../automation/algorithm.h"

#include <map>

using namespace automation::json;

namespace arduino {

#define VERSION_SIZE 10
#define CMD_ARR_MAX_ROW_COUNT 20
#define CMD_ARR_MAX_ROW_LENGTH 80

  class Eeprom {
  public:

    const size_t VERSION_OFFSET = 0;
    const size_t FORMAT_OFFSET = VERSION_SIZE;
    const size_t SERIAL_SPEED_OFFSET = VERSION_SIZE + sizeof(JsonFormat);
    const size_t SERIAL_CONFIG_OFFSET = SERIAL_SPEED_OFFSET + sizeof(unsigned long);
    const size_t CMD_COUNT_OFFSET = SERIAL_CONFIG_OFFSET + sizeof(unsigned int);
    const size_t CMD_ARR_OFFSET = CMD_COUNT_OFFSET + sizeof(unsigned int);

    JsonFormat getJsonFormat() {
      JsonFormat format;
      EEPROM.get(FORMAT_OFFSET, format);
      return format;
    }

    Eeprom &setJsonFormat(JsonFormat format) {
      EEPROM.put(FORMAT_OFFSET, format);
      return *this;
    }

    String &getVersion(String &version) {
      char buf[VERSION_SIZE];
      EEPROM.get(VERSION_OFFSET, buf);
      version = buf;
      return version;
    }

    Eeprom &setVersion(const char *pszVersion) {
      char buf[VERSION_SIZE];
      strcpy(buf,pszVersion);
      EEPROM.put(VERSION_OFFSET, buf);
      return *this;
    }

    unsigned int getCommandCount() {
      unsigned int cmdCnt;
      EEPROM.get(CMD_COUNT_OFFSET, cmdCnt);
      return cmdCnt;
    }

    int setCommandCount(unsigned int cmdCnt) {
      EEPROM.put( CMD_COUNT_OFFSET, cmdCnt);
      return CMD_OK;
    }
    
    unsigned long getSerialSpeed() {
      unsigned long rtnVal;
      EEPROM.get(SERIAL_SPEED_OFFSET, rtnVal);
      return rtnVal;
    }

    int setSerialSpeed(unsigned long v) {
      EEPROM.put( SERIAL_SPEED_OFFSET, v);
      return CMD_OK;
    }

    unsigned int getSerialConfig() {
      unsigned int rtnVal;
      EEPROM.get(SERIAL_CONFIG_OFFSET, rtnVal);
      return rtnVal;
    }

    int setSerialConfig(unsigned int v) {
      EEPROM.put( SERIAL_CONFIG_OFFSET, v);
      return CMD_OK;
    }

    int getCommandAt(int index, String &dest) {
      //cout << __PRETTY_FUNCTION__ << " index=" << index << endl;
      if (index >= getCommandCount()) {
        return INDEX_OUT_OF_BOUNDS;
      }
      char buf[CMD_ARR_MAX_ROW_LENGTH];
      EEPROM.get(CMD_ARR_OFFSET + index*CMD_ARR_MAX_ROW_LENGTH, buf);
      dest = buf;
      return CMD_OK;
    }

    int setCommandAt(int index, const char* cmd) {
      //cout << __PRETTY_FUNCTION__ << " index=" << index << ", cmd=" << cmd << "." << endl;
      if (index >= getCommandCount()) {
        return INDEX_OUT_OF_BOUNDS;
      }
      char buf[CMD_ARR_MAX_ROW_LENGTH];
      strcpy(buf,cmd);
      EEPROM.put(CMD_ARR_OFFSET + index*CMD_ARR_MAX_ROW_LENGTH, buf);
      return CMD_OK;
    }

    int removeCommandAt(int index) {
      if (index < 0 || index >= getCommandCount()) {
        return INDEX_OUT_OF_BOUNDS;
      }
      size_t cmdCnt = getCommandCount();
      for( int i = index; i < cmdCnt-1; i++ ) {
        String cmd;
        getCommandAt(i+1,cmd);
        setCommandAt(i,cmd.c_str());
      }
      setCommandCount(cmdCnt-1);
      return CMD_OK;
    }

    int addCommand(const char* cmd) {
      return insertCommandAt(-1,cmd);
    }

    int insertCommandAt(int index, const char* cmd) {
      if ( !cmd ) {
        return NULL_ARGUMENT;
      }
      size_t cmdCnt = getCommandCount();
      if ( index == -1 ) {
        index = cmdCnt; // append 
      } else if ( index < 0 || index > cmdCnt ) {
        return INDEX_OUT_OF_BOUNDS;
      } else if ( cmdCnt >= CMD_ARR_MAX_ROW_COUNT ) {
        return ARRAY_FULL;
      }
      setCommandCount(cmdCnt+1);

      // shift everything to make room
      for ( int i = cmdCnt; i > index; i-- ) {
        String tmp;
        int code = getCommandAt(i-1,tmp);
        if ( code ) {
          return code;
        }
        code = setCommandAt(i,tmp.c_str());
        if ( code ) {
          return code;
        }
      }

      setCommandAt(index,cmd);
      return CMD_OK;
    }

    int findCommand(const char* searchPattern) {
      if ( !searchPattern ) {
        return NULL_ARGUMENT;
      }
      size_t cmdCnt = getCommandCount();
      for( int i = 0; i < cmdCnt; i++ ) {
        String cmd;
        getCommandAt(i,cmd);

        if ( text::WildcardMatcher::test(searchPattern,cmd.c_str()) ) {
          return i;
        }
      }
      return NOT_FOUND;
    }

    int removeCommand(const char* searchPattern, bool bRemoveAllMatches = false) {
      int removedCnt = 0;
      int index;
      while ( (index = findCommand(searchPattern)) >= 0 ) {
        int rtnCode = removeCommandAt(index);
        if ( rtnCode != CMD_OK ) {
          return rtnCode;
        } else {
          removedCnt++;
          if ( !bRemoveAllMatches ) {
            break;
          }
        }
      };
      return removedCnt;
    }

    int replaceCommand(const char* searchPattern,const char* cmd, bool bAddIfNotFound = false) {
      int index = findCommand(searchPattern);
      if ( index == NOT_FOUND) {
        if ( bAddIfNotFound ) {
            return addCommand(cmd);
        } else {
            return NOT_FOUND;
        }
      } else {
        return setCommandAt(index,cmd);
      }
    }

    
    void noPrefixPrint(JsonStreamWriter& w) {
      w.noPrefixPrintln("{");
      w.increaseDepth();
      String str;
      w.printlnStringObj(F("version"), getVersion(str), ",");
      w.printlnNumberObj(F("serialSpeed"), getSerialSpeed(), ",");
      w.printlnStringObj(F("serialConfig"), Eeprom::serialConfigAsString(getSerialConfig()), ",");
      w.printlnStringObj(F("jsonFormat"), formatAsString(getJsonFormat()).c_str(), ",");
      int cmdCnt = getCommandCount();
      //w.printlnNumberObj(F("commandCount"), cmdCnt, ",");
      w.printKey(F("commands"));
      w.noPrefixPrintln("[");
      w.increaseDepth();
      for( int i = 0; i < cmdCnt; i++ ) {
        getCommandAt(i,str);
        w.printlnStringVal(str.c_str(),(i+1 < cmdCnt) ? "," : "");
      }
      w.decreaseDepth();
      w.println("]");
      w.decreaseDepth();
      w.print("}");
    }

    static string serialConfigAsString( unsigned int serialConfig ) {
      std::map<unsigned int,string> validConfigs = { {SERIAL_8N1,"8N1"}, {SERIAL_8E1,"8E1"}, {SERIAL_8O1,"8O1"} };
      auto it = validConfigs.find(serialConfig);
      if ( it != validConfigs.end() ) {
        return it->second;
      } else {
        return string("INVALID CONFIG:" ) + text::asString(serialConfig);
      }
    }

  protected:


  } eeprom;

}
#endif //ARDUINO_SOLAR_SKETCH_EEPROM_H
