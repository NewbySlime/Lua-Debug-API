#ifndef LUASTACK_ITER_HEADER
#define LUASTACK_ITER_HEADER

#include "lua_includes.h"


#define LUA_CONV_T2B(state, top_stack) (lua_gettop(state) + 1 + top_stack)
#define LUA_CONV_B2T(state, bottom_stack) (bottom_stack - 1 - lua_gettop(state))

#endif