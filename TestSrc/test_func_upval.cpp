#include "../Src/lua_includes.h"

#include "stdlib.h"
#include "string"


int _test_cb(lua_State* state){
  const char* _test_str = lua_tolstring(state, lua_upvalueindex(1), NULL);
  printf("test cb: '%s'\n", _test_str);
}


int main(){
  lua_State* _state = luaL_newstate();
  luaL_openlibs(_state);

  int _err = luaL_dofile(_state, "test_func_upval.lua");
  if(_err != LUA_OK){
    printf("Cannot open lua file. Err: %d\n", _err);
    exit(_err);
  }

  std::string _test = "this is a test";
  lua_pushlstring(_state, _test.c_str(), _test.size());
  lua_pushcclosure(_state, _test_cb, 1);
  lua_setglobal(_state, "test_cb");
  
  lua_getglobal(_state, "test_calling_function");
  lua_call(_state, 0, 0);

  // pops a stack
  lua_pop(_state, 1);

  lua_close(_state);
  printf("Program closed successfully.\n");
}