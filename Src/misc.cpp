#include "chrono"
#include "misc.h"



std::string create_random_name(){
  using namespace std::chrono;

  duration _time = system_clock::now().time_since_epoch();
  microseconds _ms = duration_cast<microseconds>(_time);
  seconds _s = duration_cast<seconds>(_time);

  return std::to_string(_s.count()) + std::to_string(_ms.count());
}