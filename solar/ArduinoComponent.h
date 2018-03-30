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
};
#endif