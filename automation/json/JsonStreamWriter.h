#ifndef __AUTOMATION_JSON_WRITER__
#define __AUTOMATION_JSON_WRITER__

// Do not refactor with a JSON library.  This class allows Arduino Serial port JSON to share same format as external processes.

#include "SerialByteCounter.h"
#include "OutputStreamPrinter.h"
#include "json.h"

namespace automation {
namespace json {


/**
 *  Print JSON to something with print() and println() methods such as Arduino Serial
 **/
class JsonStreamWriter
{
  protected:
  SerialByteCounter byteCounter;
  unsigned long byteCnt;
  unsigned long checksum;

  OutputStreamPrinter& impl;
  
  template<typename TPrintable>
  void updateState(TPrintable printable) 
  {
    byteCounter.checksum = 0;
    byteCnt += byteCounter.print(printable);
    checksum += byteCounter.checksum;
  }

  // All JsonStreamWriter prints should end up here.  
  template<typename TPrintable>
  void statefulPrint(TPrintable printable) { 
    updateState(printable);
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

  void clearByteCount() { byteCnt = 0; }
  unsigned long getByteCount() { return byteCnt; }
  void clearChecksum() { checksum = 0; }
  unsigned long getChecksum() { return checksum; }

  int depth;
  long beginStringObjByteCnt;

  JsonStreamWriter(OutputStreamPrinter& impl, int depth = 0) :
    impl(impl),
    depth(depth)
  {
  }

  JsonStreamWriter& printPrefix() { 
    if ( json::isPretty() ) {
      for (int i = 0; i < depth; i++) {
        statefulPrint("  "); 
      }
    } 
    return *this; 
  }

  template<typename TPrintable>
  JsonStreamWriter& print(TPrintable printable) 
  { 
    printPrefix(); 
    statefulPrint(printable); 
    return *this; 
  } 

  template<typename TPrintable>
  JsonStreamWriter& println(TPrintable printable)
  { 
    print(printable); 
    if ( json::isPretty() ){
      statefulPrintln(); 
    }
    return *this; 
  } 

  JsonStreamWriter& println()
  {
    if ( json::isPretty() ){
      statefulPrintln(); 
    }
    return *this;
  }

  template<typename TPrintable>
  JsonStreamWriter& noPrefixPrint(TPrintable printable)
  { 
    statefulPrint(printable); 
    return *this; 
  } 

  template<typename TPrintable>
  JsonStreamWriter& noPrefixPrintln(TPrintable printable)
  { 
    statefulPrint(printable); 
    if ( json::isPretty() ){
      statefulPrintln(); 
    }
    return *this; 
  }

  JsonStreamWriter& noPrefixPrintln()
  { 
    if ( json::isPretty() ){
      statefulPrintln(); 
    }
    return *this; 
  } 

  // Track indentation for pretty print
  JsonStreamWriter& increaseDepth() { depth++; return *this; }
  JsonStreamWriter& decreaseDepth() { depth--; return *this; }

  template<typename TKey>
  JsonStreamWriter& printKey(TKey k)
  { 
    printPrefix(); 
    statefulPrint("\""); 
    statefulPrint(k);
    statefulPrint("\": "); 
    return *this;
  }

  template<typename TKey, typename TVal>
  JsonStreamWriter& printStringObj(TKey k, TVal v, const char* suffix = "" )
  {     
    beginStringObj(k);
    appendStringVal(v);
    endStringObj(suffix);
    return *this;
  }

  template<typename TKey>
  JsonStreamWriter& beginStringObj(TKey k)
  {     
    printKey(k);
    statefulPrint("\""); 
    beginStringObjByteCnt = byteCnt;
    return *this;
  }

  template<typename TVal>
  JsonStreamWriter& operator+(TVal v) 
  {
    return appendStringVal(v);
  }

  JsonStreamWriter& appendStringVal(const std::string& str)
  { 
    statefulPrint(str.c_str()); 
    return *this;
  }

  //TODO - escape double quotes if a string
  template<typename TVal>
  JsonStreamWriter& appendStringVal(TVal v)
  { 
    statefulPrint(v); 
    return *this;
  }
  
  JsonStreamWriter& endStringObj(const char* suffix = "")
  { 
    statefulPrint("\"");
    statefulPrint(suffix);
    beginStringObjByteCnt = -1;
    return *this;
  }

  template<typename TVal>
  JsonStreamWriter& printStringVal(TVal v, const char* suffix = "")
  { 
    printPrefix();
    statefulPrint("\"");
    statefulPrint(v);
    statefulPrint("\"");
    statefulPrint(suffix);
    return *this;
  }

  template<typename TVal>
  JsonStreamWriter& printlnStringVal(TVal v, const char* suffix = "")
  { 
    printStringVal(v,suffix);
    println();
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
  JsonStreamWriter& printlnStringObj(TKey k, TVal v, const char* suffix = "" )
  {
    printStringObj(k,v,suffix);
    println();
    return *this;
  }

  template<typename TKey, typename TVal>
  JsonStreamWriter& printNumberObj(TKey k, TVal v, const char* suffix = "" )
  {     
    printKey(k);
    statefulPrint(v); 
    statefulPrint(suffix);
    return *this;
  }

  template<typename TKey, typename TVal>
  JsonStreamWriter& printlnNumberObj(TKey k, TVal v, const char* suffix = "" )
  {     
    printNumberObj(k,v,suffix);
    println();
    return *this;
  }

  template<typename TKey>
  JsonStreamWriter& printBoolObj(TKey k, bool v, const char* suffix = "" )
  {     
    printKey(k);
    statefulPrint(v?F("true"):F("false")); 
    statefulPrint(suffix);
    return *this;
  }

  template<typename TKey>
  JsonStreamWriter& printlnBoolObj(TKey k, bool v, const char* suffix = "" )
  {
    printBoolObj(k,v,suffix);
    println();
    return *this;
  }

  template<typename TKey, typename TArray> 
  JsonStreamWriter& printlnArrayObj(TKey k, TArray arr, const char* suffix = "" ) 
  {
    printArrayObj(k,arr,suffix);
    println();
    return *this;
  }

  template<typename TKey, typename TIterator>
  JsonStreamWriter& printIteratorObj(TKey k, TIterator itr, TIterator endItr, 
      const char* suffix = "", bool bVerbose = false ) {
    printKey(k);
    noPrefixPrint("[");
    increaseDepth();

    bool bFirst = true;
    while( itr != endItr )
    {
      if ( bFirst ) {
        bFirst = false;
        noPrefixPrintln();
      } else {
        noPrefixPrintln(",");
      }
      (*itr)->print(*this,bVerbose);
      automation::threadKeepAliveReset();
      itr++;
    };
    noPrefixPrintln();
    decreaseDepth();
    print("]");
    noPrefixPrint(suffix);
    return *this;
  }

  template<typename TKey, typename TIterator>
  JsonStreamWriter& printlnIteratorObj(TKey k, TIterator itr, TIterator endItr, 
    const char* suffix = "", bool bVerbose = false ) {
    printIteratorObj(k,itr,endItr,suffix,bVerbose);
    println();
    return *this;
  }

  template<typename TKey, typename TArray>
  JsonStreamWriter& printVectorObj(TKey k, TArray arr, const char* suffix = "", bool bVerbose = false )
  {
    return printIteratorObj(k,arr.begin(),arr.end(),suffix,bVerbose);
  }

  template<typename TKey, typename TArray>
  JsonStreamWriter& printlnVectorObj(TKey k, TArray arr, const char* suffix = "", bool bVerbose = false )
  {
    printVectorObj(k,arr,suffix,bVerbose);
    println();
    return *this;
  }

  // Allow printing directly to Serial or other writer without counting bytes or updating checksum
  template<typename TPrintable>
  JsonStreamWriter& implPrint(TPrintable printable)
  { 
    impl.print(printable); 
    return *this; 
  } 

  template<typename TPrintable>
  JsonStreamWriter& implPrintln(TPrintable printable)
  { 
    impl.println(printable); 
    return *this; 
  }

};

class JsonSerialWriter : public JsonStreamWriter
{
public:
  JsonSerialWriter(int depth = 0) : JsonStreamWriter(stdoutStreamPrinter, depth) {}
};

}}
#endif
