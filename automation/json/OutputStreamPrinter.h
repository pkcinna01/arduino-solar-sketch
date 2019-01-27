#ifndef _AUTOMATION_JSON_OUTPUT_STREAM_PRINTER_
#define _AUTOMATION_JSON_OUTPUT_STREAM_PRINTER_

#include "../Automation.h"

#include <iostream>
#include <sstream>

namespace automation {
namespace json {

#ifdef ARDUINO_APP
  std::ostream &operator<<(std::ostream &os, const __FlashStringHelper *pFlashStringHelper)
  {
    os << String(pFlashStringHelper).c_str();
    return os;
  }

  std::ostream &operator<<(std::ostream &os, String &str)
  {
    os << str.c_str();
    return os;
  }

  std::ostream &operator<<(std::ostream &os, uint8_t val)
  {
    unsigned int uiVal = val;
    os << uiVal;
    return os;
  }
#endif

// Arduino Serial has lots of print function signatures so using template instead of virtual functions
struct OutputStreamPrinter
{
  std::ostream *pOs;
  OutputStreamPrinter(std::ostream *pOs = nullptr) : pOs(pOs) {}
  template <typename T>
  void print(T &val)
  {
    if (pOs)
      (*pOs) << val;
  }
  template <typename T>
  void println(T &val)
  {
    if (pOs)
      (*pOs) << val << std::endl;
  }
};

struct StringStreamPrinter : public OutputStreamPrinter
{
  std::stringstream ss;
  StringStreamPrinter() : OutputStreamPrinter(&ss) {}
};

// ArduinoSTL library has std::cout going to HardwareSerial
struct StdoutStreamPrinter : OutputStreamPrinter
{
  StdoutStreamPrinter() : OutputStreamPrinter(&std::cout) {}
};

static StdoutStreamPrinter stdoutStreamPrinter;

struct NullStreamPrinter : public OutputStreamPrinter
{
  NullStreamPrinter() : OutputStreamPrinter(&automation::cnull) {}
};

static NullStreamPrinter nullStreamPrinter;

}}

#endif