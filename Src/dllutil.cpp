#include "dllutil.h"


using namespace dynamic_library::util;


destructor_helper::destructor_helper(destructor_cb cb){
  _cb = cb;
}

destructor_helper::~destructor_helper(){
  _cb();
}