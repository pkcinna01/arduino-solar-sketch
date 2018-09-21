#ifndef ARDUINO_STATUS_H
#define ARDUINO_STATUS_H

#include "Arduino.h"

#include "../automation/Automation.h"
#include "../automation/Sensor.h"

using namespace arduino;

struct Status {

  String msg;
  int code;

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

  void print(int depth = 0) {
    JsonSerialWriter w(depth);
    w.println("{");
    w.increaseDepth();
    w.printlnNumberObj(F("code"),code,",");
    w.printlnStringObj(F("msg"),msg);
    w.decreaseDepth();
    w.println("}");
   }

};

#endif