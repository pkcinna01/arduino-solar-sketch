#ifndef ARDUINO_SOLAR_SKETCH_ARDUINO_H
#define ARDUINO_SOLAR_SKETCH_ARDUINO_H

#include <Time.h>
#include <avr/wdt.h>

namespace arduino {
  const float Vref = 5.0;  //TODO read from mega board

  String gLastErrorMsg;
  String gLastInfoMsg;

  typedef unsigned char JsonFormat;
  const JsonFormat JSON_FORMAT_INVALID = -1, JSON_FORMAT_COMPACT = 0, JSON_FORMAT_PRETTY = 1;

  JsonFormat jsonFormat = JSON_FORMAT_PRETTY;

  JsonFormat parseFormat( const char* pszFormat) {
    if (!strcmp_P(pszFormat, PSTR("JSON_COMPACT"))) {
      return JSON_FORMAT_COMPACT;
    } else if (!strcmp_P(pszFormat, PSTR("JSON_PRETTY"))) {
      return JSON_FORMAT_PRETTY;
    } else {
      return JSON_FORMAT_INVALID;
    }
  }

  String formatAsString(JsonFormat fmt)
  {
    if ( fmt == JSON_FORMAT_COMPACT ) {
      return "JSON_COMPACT";
    } else if ( fmt == JSON_FORMAT_PRETTY ){
      return "JSON_PRETTY";
    } else {
      String msg("INVALID:");
      msg += (unsigned int) fmt;
      return msg;
    }
  }

  namespace watchdog {
    const auto KeepAliveMs = WDTO_8S;
    bool resetRequested = false;
        
    void enable() {
      wdt_enable(KeepAliveMs);
    }

    void keepAlive() {
      while( resetRequested ); // this will lock program and prevent wdt_reset which will cause reboot
      wdt_reset();
    }
  }

  long readVcc() {
    // Read 1.1V reference against AVcc
    // set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
#else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif

#if defined(__AVR_ATmega2560__)
    ADCSRB &= ~_BV(MUX5);
#endif

    delay(2); // Wait for Vref to settle
    ADCSRA |= _BV(ADSC); // Start conversion
    while (bit_is_set(ADCSRA,ADSC)); // measuring

    uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
    uint8_t high = ADCH; // unlocks both

    long result = (high<<8) | low;

    result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
    return result; // Vcc in millivolts
  }


  std::ostream &operator<<(std::ostream &os, const __FlashStringHelper *s) {
    String str(s);
    os << str.c_str();
    return os;
  }

#define CMD_OK 0
#define CMD_ERROR -1
#define INDEX_OUT_OF_BOUNDS -2
#define EEPROM_CMD_ARRAY_FULL -3
#define EEPROM_CMD_NOT_FOUND -4
#define INVALID_CMD_ARGUMENT -5
#define NO_NAME_MATCHES -6
#define NULL_SEARCH_PATTERN -7
#define NULL_ARGUMENT -8


  String errorDesc( const String& context, int errorCode ) {
    String rtn(context);
    rtn += (errorCode == CMD_OK ? F(" OK: ") : F(" ERROR: "));
    switch ( errorCode ) {
      case CMD_ERROR:
        rtn += F("CMD_ERROR");
        break;
      case INDEX_OUT_OF_BOUNDS:
        rtn += F("INDEX_OUT_OF_BOUNDS");
        break;
      case EEPROM_CMD_ARRAY_FULL:
        rtn += F("EEPROM_CMD_ARRAY_FULL");
        break;
      case EEPROM_CMD_NOT_FOUND:
        rtn += F("EEPROM_CMD_NOT_FOUND");
        break;
      case INVALID_CMD_ARGUMENT:
        rtn += F("INVALID_CMD_ARGUMENT");
        break;
      case NULL_SEARCH_PATTERN:
        rtn += F("NULL_SEARCH_PATTERN");
        break;
      case NULL_ARGUMENT:
        rtn += F("NULL_ARGUMENT");
        break;
      default:
        ;
    };
    rtn += F("(");
    rtn += errorCode;
    rtn += F(")");
    return rtn;
  }

}
#endif //ARDUINO_SOLAR_SKETCH_ARDUINO_H
