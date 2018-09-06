#include "Sensor.h"

namespace automation {

  float Sensor::average(const vector<Sensor*>& sensors) {
    float total, sensorCnt = sensors.size();
    for ( Sensor* s : sensors) total += s->getValue();
    return total/sensorCnt;
  }

  bool Sensor::compareValues(const Sensor* lhs, const Sensor* rhs){
    return lhs->getValue() < rhs->getValue();
  }

  float Sensor::minimum(const vector<Sensor*>& sensors) {
    auto it = std::min_element( sensors.begin(), sensors.end(), Sensor::compareValues);
    return it == sensors.end() ? 0 : (*it)->getValue();
  }

  float Sensor::maximum(const vector<Sensor*>& sensors) {
    auto it = std::max_element( sensors.begin(), sensors.end(), Sensor::compareValues);
    return it == sensors.end() ? 0 : (*it)->getValue();
  }

}

