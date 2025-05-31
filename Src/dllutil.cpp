#include "dllutil.h"


using namespace dynamic_library::util;




// MARK: constructor_helper definition

constructor_helper::constructor_helper(constructor_cb cb){
  cb();
}




// MARK: destructor_helper definition

destructor_helper::destructor_helper(destructor_cb cb){
  _cb = cb;
}

destructor_helper::~destructor_helper(){
  _cb();
}