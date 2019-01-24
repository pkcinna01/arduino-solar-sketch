#ifndef AUTOMATION_SERIAL_BYTE_COUNTER_H
#define AUTOMATION_SERIAL_BYTE_COUNTER_H

#include <cstring>

namespace automation { namespace json {

  class NullByteCounter {
    public:
    unsigned long checksum;
    template<typename TPrintable>
    size_t print(TPrintable p) { 
      return 0; 
    }
    template<typename TPrintable>
    size_t println(TPrintable p) { return 0; }
  };

  #ifdef ARDUINO_APP

    // Utility to count bytes sent with Serial class
    class SerialByteCounter : public Print {
      public:
      unsigned long checksum;
      // All the Print::print... methods call write so we can track bytes sent
      virtual size_t write(uint8_t c) 
      {
        checksum += c;
        return 1;
      }
    };

  #else

    // Counting bytes not supported if not Arduino Serial output    

    using SerialByteCounter = NullByteCounter;

  #endif

}}
#endif
