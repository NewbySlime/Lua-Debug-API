#ifndef LUA_GENERAL_USAGE_RUNTIME_HEADER
#define LUA_GENERAL_USAGE_RUNTIME_HEADER

#include "luaapi_core.h"
#include "luaincludes.h"

namespace lua::utility{
#ifdef LUA_CODE_EXISTS
  // Keep in mind that this is minimally configured runtime.
  lua_State* require_general_usage_runtime();
#endif
}

#endif