#ifndef ARDUINO_SOLAR_SKETCH_EEPROM_H
#define ARDUINO_SOLAR_SKETCH_EEPROM_H

#include "Arduino.h"

namespace arduino {

#define VERSION_SIZE 10
#define CMD_ARR_MAX_ROW_COUNT 20
#define CMD_ARR_MAX_ROW_LENGTH 80

  class Eeprom {
  public:

    const size_t VERSION_OFFSET = 0;
    const size_t FORMAT_OFFSET = VERSION_SIZE;
    const size_t CMD_COUNT_OFFSET = VERSION_SIZE + sizeof(JsonFormat);
    const size_t CMD_ARR_OFFSET = CMD_COUNT_OFFSET + sizeof(size_t);

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

    size_t getCommandCount() {
      size_t cmdCnt;
      EEPROM.get(CMD_COUNT_OFFSET, cmdCnt);
      return cmdCnt;
    }

    int setCommandCount(size_t cmdCnt) {
      EEPROM.put( CMD_COUNT_OFFSET, cmdCnt);
      return CMD_OK;
    }

    int getCommandAt(int index, String &dest) {
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
      size_t cmdCnt = getCommandCount();
      if ( cmdCnt >= CMD_ARR_MAX_ROW_COUNT ) {
        return EEPROM_CMD_ARRAY_FULL;
      } else {
        cmdCnt++;
        setCommandCount(cmdCnt);
        setCommandAt(cmdCnt-1,cmd);
        cout << __PRETTY_FUNCTION__ << " added command '" << cmd << "' at index " << cmdCnt-1 << endl;        
      }
      return CMD_OK;
    }

    int findCommand(const char* searchPattern) {
      size_t cmdCnt = getCommandCount();
      for( int i = 0; i < cmdCnt; i++ ) {
        String cmd;
        getCommandAt(i,cmd);

        if ( automation::WildcardMatcher::test(searchPattern,cmd.c_str()) ) {
          return i;
        }
      }
      return EEPROM_CMD_NOT_FOUND;
    }

    int removeCommand(const char* searchPattern) {
      int index = findCommand(searchPattern);
      if ( index != EEPROM_CMD_NOT_FOUND ) {
        return removeCommandAt(index);
      } else {
        return EEPROM_CMD_NOT_FOUND;
      }
    }

    void print(int depth=0) {
      JsonSerialWriter w(depth);
      w.print("");
      noPrefixPrint(depth);
    }

    void noPrefixPrint(int depth=0) {
      JsonSerialWriter w(depth);
      w.noPrefixPrintln("{");
      w.increaseDepth();
      String str;
      w.printlnStringObj(F("version"), getVersion(str), ",");
      w.printlnStringObj(F("jsonFormat"), arduino::formatAsString(getJsonFormat()), ",");
      int cmdCnt = getCommandCount();
      w.printlnNumberObj(F("commandCount"), cmdCnt, ",");
      w.printKey(F("commands"));
      w.noPrefixPrintln("[");
      w.increaseDepth();
      for( int i = 0; i < cmdCnt; i++ ) {
        if ( i > 0 ) {
          w.noPrefixPrintln(",");
        }
        getCommandAt(i,str);
        w.printStringObj(F("command"), str);
      }
      //w.noPrefixPrintln(); bug - this should not cause checksum error
      w.decreaseDepth();
      w.println("]");
      w.decreaseDepth();
      w.print("}");
    }

  protected:


  } eeprom;

}
#endif //ARDUINO_SOLAR_SKETCH_EEPROM_H
