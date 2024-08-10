#ifndef LUA_STR_HEADER
#define LUA_STR_HEADER

#include "lua_includes.h"
#include "lua_variant.h"

#include "string"


namespace lua{
  std::string to_string(lua_State* state, int stack_idx);
}

#endif