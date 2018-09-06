#ifndef JSONWRITER_H
#define JSONWRITER_H

// Utility to count bytes sent with Serial class
class SerialByteCounter : public Print {
  public:
  unsigned long checksum;
  virtual size_t write(uint8_t c) 
  {
    checksum += c;
    return 1;
  }
};

SerialByteCounter byteCounter;

typedef unsigned char JsonFormat;
const JsonFormat JSON_FORMAT_INVALID = -1, JSON_FORMAT_COMPACT = 0, JSON_FORMAT_PRETTY = 1;

/**
 *  Print JSON to something with print() and println() methods such as Arduino Serial
 **/
template<class T>
class JsonWriter
{
  protected:

  static unsigned long byteCnt;
  static unsigned long checksum;

  T& impl;
  
  template<typename TPrintable>
  static void updateState(TPrintable printable) 
  {
    byteCounter.checksum = 0;
    byteCnt += byteCounter.print(printable);
    checksum += byteCounter.checksum;
  }

  // All JsonWriter prints should end up here.  
  template<typename TPrintable>
  void statefulPrint(TPrintable printable) { 
    JsonWriter<T>::updateState(printable);
    impl.print(printable);
  }

  void statefulPrintln() { 
    statefulPrint("\n");
  }

  template<typename TPrintable>
  void statefulPrintln(TPrintable printable) { 
    statefulPrint(printable);
    statefulPrintln();
  }

  public:
  
  static JsonFormat format;

  static void clearByteCount() { byteCnt = 0; }
  static unsigned long getByteCount() { return byteCnt; }
  static void clearChecksum() { checksum = 0; }
  static unsigned long getChecksum() { return checksum; }
  static bool isPretty() { return format == JSON_FORMAT_PRETTY; }
  

  int depth;
  int beginStringObjByteCnt;

  JsonWriter<T>(T& impl, int depth = 0) :
    impl(impl),
    depth(depth)
  {
  }

  JsonWriter& printPrefix() { 
    if ( isPretty() ) {
      for (int i = 0; i < depth; i++) {
        statefulPrint("  "); 
      }
    } 
    return *this; 
  }

  template<typename TPrintable>
  JsonWriter& print(TPrintable printable) 
  { 
    printPrefix(); 
    statefulPrint(printable); 
    return *this; 
  } 

  template<typename TPrintable>
  JsonWriter& println(TPrintable printable)
  { 
    print(printable); 
    if ( isPretty() ){
      statefulPrintln(); 
    }
    return *this; 
  } 

  JsonWriter& println()
  {
    if ( isPretty() ){
      statefulPrintln(); 
    }
  }

  template<typename TPrintable>
  JsonWriter& noPrefixPrint(TPrintable printable)
  { 
    statefulPrint(printable); 
    return *this; 
  } 

  template<typename TPrintable>
  JsonWriter& noPrefixPrintln(TPrintable printable)
  { 
    statefulPrint(printable); 
    if ( isPretty() ){
      statefulPrintln(); 
    }
    return *this; 
  } 

  // Track indentation for pretty print
  JsonWriter& increaseDepth() { depth++; return *this; }
  JsonWriter& decreaseDepth() { depth--; return *this; }

  template<typename TKey>
  JsonWriter& printKey(TKey k)
  { 
    printPrefix(); 
    statefulPrint("\""); 
    statefulPrint(k);
    statefulPrint("\": "); 
    return *this;
  }

  template<typename TKey, typename TVal>
  JsonWriter& printStringObj(TKey k, TVal v, const char* suffix = "" )
  {     
    beginStringObj(k);
    appendStringVal(v);
    endStringObj(suffix);
    return *this;
  }

  template<typename TKey>
  JsonWriter& beginStringObj(TKey k)
  {     
    printKey(k);
    statefulPrint("\""); 
    beginStringObjByteCnt = byteCnt;
    return *this;
  }

  template<typename TVal>
  JsonWriter operator+(TVal v) 
  {
    return appendStringVal(v);
  }

  //TODO - escape double quotes if a string
  template<typename TVal>
  JsonWriter& appendStringVal(TVal v)
  { 
    statefulPrint(v); 
    return *this;
  }
  
  JsonWriter& endStringObj(const char* suffix = "")
  { 
    statefulPrint("\"");
    statefulPrint(suffix);
    beginStringObjByteCnt = -1;
    return *this;
  }

  int getOpenStringValByteCnt()
  {
    if ( beginStringObjByteCnt < 0 ) {
      // not currently writing a string value;
      return -1;
    }
    return byteCnt - beginStringObjByteCnt;
  }

  template<typename TKey, typename TVal>
  JsonWriter& printlnStringObj(TKey k, TVal v, const char* suffix = "" )
  {
    printStringObj(k,v,suffix);
    println();
    return *this;
  }

  template<typename TKey, typename TVal>
  JsonWriter& printNumberObj(TKey k, TVal v, const char* suffix = "" )
  {     
    printKey(k);
    statefulPrint(v); 
    statefulPrint(suffix);
    return *this;
  }

  template<typename TKey, typename TVal>
  JsonWriter& printlnNumberObj(TKey k, TVal v, const char* suffix = "" )
  {     
    printNumberObj(k,v,suffix);
    println();
    return *this;
  }

  template<typename TKey>
  JsonWriter& printBoolObj(TKey k, bool v, const char* suffix = "" )
  {     
    printKey(k);
    statefulPrint(v?"true":"false"); 
    statefulPrint(suffix);
    return *this;
  }

  template<typename TKey>
  JsonWriter& printlnBoolObj(TKey k, bool v, const char* suffix = "" )
  {
    printBoolObj(k,v,suffix);
    println();
    return *this;
  }

  template<typename TKey, typename TArray> 
  JsonWriter& printArrayObj(TKey k, TArray arr, const char* suffix = "" ) 
  {
    printKey(k);
    noPrefixPrintln("[");

    for( int i = 0; arr[i] != NULL; i++ )
    {
      if ( i != 0 )
      {
        noPrefixPrintln(",");
      }
      arr[i]->print(depth+1);
    };
    println();
    print("]");
    noPrefixPrint(suffix);
    return *this;
  }

  template<typename TKey, typename TArray> 
  JsonWriter& printlnArrayObj(TKey k, TArray arr, const char* suffix = "" ) 
  {
    printArrayObj(k,arr,suffix);
    println();
    return *this;
  }

  // Allow printing directly to Serial or other writer without counting bytes or updating checksum
  template<typename TPrintable>
  JsonWriter& implPrint(TPrintable printable)
  { 
    impl.print(printable); 
    return *this; 
  } 

  template<typename TPrintable>
  JsonWriter& implPrintln(TPrintable printable)
  { 
    impl.println(printable); 
    return *this; 
  } 

};

class JsonSerialWriter : public JsonWriter<HardwareSerial>
{
  public:
  JsonSerialWriter(int depth=0) :
    JsonWriter(Serial,depth)
  {
  }
};

template<class C>
unsigned long JsonWriter<C>::checksum = 0;
template<class C>
unsigned long JsonWriter<C>::byteCnt = 0;
template<class C>
JsonFormat JsonWriter<C>::format = JSON_FORMAT_PRETTY;

#endif