#ifndef ARDUINO_SOLAR_SKETCH_ARDUINO_H
#define ARDUINO_SOLAR_SKETCH_ARDUINO_H

#include <Time.h>

namespace arduino {
  const float Vref = 5.0;  //TODO read from mega board
  const unsigned char VERSION_SIZE = 10;


  typedef unsigned char PersistMode;

  const PersistMode PERSIST_MODE_INVALID = -1, PERSIST_MODE_TRANSIENT = 0, PERSIST_MODE_SAVE = 1;

  PersistMode parsePersistMode(const char* pszPersistMode)
  {
    if ( !strcmp_P(pszPersistMode,PSTR("PERSIST")) )
    {
      return PERSIST_MODE_SAVE;
    }
    else if ( !strcmp_P(pszPersistMode,PSTR("TRANSIENT")) )
    {
      return PERSIST_MODE_TRANSIENT;
    }
    else
    {
      return PERSIST_MODE_INVALID;
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


}
#endif //ARDUINO_SOLAR_SKETCH_ARDUINO_H
