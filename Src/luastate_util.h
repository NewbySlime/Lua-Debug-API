#ifndef LUASTATE_UTIL_HEADER
#define LUASTATE_UTIL_HEADER

#include "luaincludes.h"


namespace lua{
  // Use memdynamic_management for the allocator. Will not change the Lua code, in case of users needed to use Lua's default allocator.
  // Close the state using lua_close().
  lua_State* newstate();
}


void luastate_util_initstate(lua_State*);
void luastate_util_deinitstate(lua_State*);

#endif