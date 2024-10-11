#ifndef LUASTATE_UTIL_HEADER
#define LUASTATE_UTIL_HEADER

#include "luaincludes.h"


namespace lua{
  lua_State* newstate(lua_Alloc f, void* ud);
  lua_State* newstate();
}

#endif