#include "dllutil.h"
#include "luageneral_usage_runtime.h"
#include "luastate_util.h"

using namespace dynamic_library::util;
using namespace lua;
using namespace lua::utility;


#ifdef LUA_CODE_EXISTS

static void _code_init();
static void _code_deinit();
destructor_helper _dh(_code_deinit);

static lua_State* _general_state = NULL;


static void _code_init(){
  if(!_general_state)
    _general_state = newstate();
}

static void _code_deinit(){
  if(_general_state){
    lua_close(_general_state);
    _general_state = NULL;
  }
}


lua_State* lua::utility::require_general_usage_runtime(){
  _code_init();
  return _general_state;
}


#endif