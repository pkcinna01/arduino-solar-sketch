#ifndef ARDUINOCOMPONENT_H
#define ARDUINOCOMPONENT_H

class ArduinoComponent
{
  public:
  
  virtual void setup() {};

  virtual void print(int depth) = 0;

  template<typename ObjectPtr,typename MethodPtr> 
  static float sample(ObjectPtr obj, MethodPtr method, unsigned int cnt = 5, unsigned int intervalMs = 50) 
  {
    float sum = 0;
    for (int i = 0; i < cnt; i++)
    {
      sum += (obj->*method)();
      if ( cnt > 1 ) 
      {
        delay(intervalMs);
      }
    }
    return sum/cnt;
  }

  // for debugging non-printable characters in serialbus input
  /*
  static String& appendHex(const char* pszSource, String& dest)
  {
    uint8_t length = strlen(pszSource);
    for (uint8_t i=0; i < length; i++) 
    {
        if ( i != 0 ) 
          dest += ",";
        byte byteVal = ((pszSource[i] >> 4) & 0x0F) + 48;
        char msd = (byteVal > 9) ? byteVal + (byte)7 : byteVal;
        byteVal = (pszSource[i] & 0x0F) + 48;
        char lsd = (byteVal > 9) ? byteVal + (byte)7 : byteVal;
        dest += "0x";
        dest += msd;
        dest += lsd;     
    }
    return dest;
  }
  */

};
#endif