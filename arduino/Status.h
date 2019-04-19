#ifndef ARDUINO_STATUS_H
#define ARDUINO_STATUS_H

#include "Arduino.h"

#include "../automation/Automation.h"
#include "../automation/sensor/Sensor.h"

using namespace arduino;

struct Status {

  String msg;

  int code { 0 };

  String& error(const char* msg, int code = -1) {
    this->code = code;
    this->msg = msg;
    return this->msg;
  }

  String& error(String& msg, int code = -1) {
    return this->error(msg.c_str(),code);
  }

  bool ok() { return code == 0; }

  void reset() {
    msg = "";
    code = 0;
  }

  void noPrefixPrint(JsonStreamWriter& w, bool bVerbose=false) { 
    print(w,true,false);
  }

  void print(JsonStreamWriter& w, bool bVerbose=false, bool bIncludePrefix=true) {
    
    if ( bIncludePrefix ) w.println("{"); else w.noPrefixPrintln("{");

    w.increaseDepth();
    w.printlnNumberObj(F("code"),code,",");
    w.printlnStringObj(F("msg"),msg);
    w.decreaseDepth();
    w.print("}");
   }

};

#endif
