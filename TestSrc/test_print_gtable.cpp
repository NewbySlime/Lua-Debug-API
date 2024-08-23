#include "../Src/lua_includes.h"
#include "../Src/lua_str.h"

#include "iostream"



int main(){
  lua_State* _state = luaL_newstate();
  luaL_openlibs(_state);

  lua_pushglobaltable(_state);
  std::string _global_str = lua::to_string(_state, -1);
  lua_pop(_state, 1);

  std::cout << _global_str << std::endl;

  lua_close(_state);
}