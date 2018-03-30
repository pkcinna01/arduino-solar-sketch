#ifndef JSONWRITER_H
#define JSONWRITER_H

/**
 *  Print JSON to something with print() and println() methods such as Arduino Serial
 **/
template<class T>
class JsonWriter
{
  public:
  
  int depth;

  T& impl;

  JsonWriter<T>(T& impl, int depth = 0) :
    impl(impl),
    depth(depth)
  {
  }

  bool isPretty()
  {
    return outputFormat == JSON_PRETTY;
  }

  JsonWriter& printPrefix() { 
    if ( isPretty() ) {
      for (int i = 0; i < depth; i++) {
        impl.print("  "); 
      }
    } 
    return *this; 
  }

  template<typename TPrintable>
  JsonWriter& print(TPrintable printable) 
  { 
   printPrefix(); 
    impl.print(printable); 
    return *this; 
  } 

  template<typename TPrintable>
  JsonWriter& println(TPrintable printable)
  { 
    print(printable); 
    if ( isPretty() ){
      impl.println(); 
    }
    return *this; 
  } 

  template<typename TPrintable>
  JsonWriter& appendPrintln(TPrintable printable)
  { 
    impl.print(printable); 
    if ( isPretty() ){
      impl.println(); 
    }
    return *this; 
  } 
  template<typename TPrintable>
  JsonWriter& appendPrint(TPrintable printable)
  { 
    impl.print(printable); 
    return *this; 
  } 

  JsonWriter& println()
  {
    println("");
  }
  
  JsonWriter& increaseDepth() { depth++; return *this; }
  JsonWriter& decreaseDepth() { depth--; return *this; }

  template<typename TKey>
  JsonWriter& printKey(TKey k)
  { 
    printPrefix(); 
    impl.print("\""); 
    impl.print(k);
    impl.print("\": "); 
    return *this;
  }

  template<typename TKey, typename TVal>
  JsonWriter& printStringObj(TKey k, TVal v, const char* suffix = "" )
  {     
    printKey(k);
    impl.print("\""); 
    impl.print(v); 
    impl.print("\"");
    impl.print(suffix);
    return *this;
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
    impl.print(v); 
    impl.print(suffix);
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
    impl.print(v?"true":"false"); 
    impl.print(suffix);
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
    appendPrintln("[");

    for( int i = 0; arr[i] != NULL; i++ )
    {
      if ( i != 0 )
      {
        appendPrintln(",");
      }
      arr[i]->print(depth+1);
    };
    println();
    print("]");
    impl.print(suffix);
    return *this;
  }
  template<typename TKey, typename TArray> 
  JsonWriter& printlnArrayObj(TKey k, TArray arr, const char* suffix = "" ) 
  {
    printArrayObj(k,arr,suffix);
    println();
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

#endif