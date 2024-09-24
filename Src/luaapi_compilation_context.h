#ifndef LUAAPI_COMPILATION_CONTEXT_HEADER
#define LUAAPI_COMPILATION_CONTEXT_HEADER

#include "lua_includes.h"
#include "lua_variant.h"
#include "luaapi_execution.h"
#include "luaapi_memory.h"
#include "luaapi_stack.h"
#include "luaapi_state.h"
#include "luaapi_util.h"
#include "luaapi_value.h"
#include "luatable_util.h"


namespace lua::api{
  struct compilation_context{
    compilation_context(I_execution* execution, I_memory* memory, I_stack* stack, I_state* state, I_util* util, I_value* value, I_table_util* tableutil, I_variant_util* varutil);

    I_execution* api_execution;
    I_memory* api_memory;
    I_stack* api_stack;
    I_state* api_state;
    I_util* api_util;
    I_value* api_value;
    I_table_util* api_tableutil;
    I_variant_util* api_varutil;
  };
}

#define CPPLUA_GET_API_COMPILATION_CONTEXT cpplua_get_api_compilation_context
#define CPPLUA_GET_API_COMPILATION_CONTEXT_STR MACRO_TO_STR_EXP(cpplua_get_api_compilation_context)

typedef const lua::api::compilation_context* (__stdcall *get_api_compilation_context)();
DLLEXPORT const lua::api::compilation_context* CPPLUA_GET_API_COMPILATION_CONTEXT();

#endif