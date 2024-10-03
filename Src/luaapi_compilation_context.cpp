#include "luaapi_compilation_context.h"
#include "luaapi_debug.h"
#include "luaapi_execution.h"
#include "luaapi_internal.h"
#include "luaapi_memory.h"
#include "luaapi_object_util.h"
#include "luaapi_stack.h"
#include "luaapi_state.h"
#include "luaapi_util.h"
#include "luaapi_value.h"
#include "luaapi_table_util.h"
#include "luaapi_variant_util.h"


using namespace lua::api;


#ifdef LUA_CODE_EXISTS

compilation_context::compilation_context(I_debug* debug, I_execution* execution, I_internal* internal, I_memory* memory, I_object_util* objutil, I_stack* stack, I_state* state, I_util* util, I_value* value, I_table_util* tableutil, I_variant_util* varutil){
  api_debug = debug;
  api_execution = execution;
  api_internal = internal;
  api_memory = memory;
  api_objutil = objutil;
  api_stack = stack;
  api_state = state;
  api_util = util;
  api_value = value;
  api_tableutil = tableutil;
  api_varutil = varutil;
}


static const lua::api::compilation_context __compilation_context(
  cpplua_get_api_debug_definition(),
  cpplua_get_api_execution_defintion(),
  cpplua_get_api_internal_definition(),
  cpplua_get_api_memory_definition(),
  cpplua_get_api_object_util_definition(),
  cpplua_get_api_stack_definition(),
  cpplua_get_api_state_definition(),
  cpplua_get_api_util_definition(),
  cpplua_get_api_value_definition(),
  cpplua_get_api_table_util_definition(),
  cpplua_get_api_variant_util_definition()
);

DLLEXPORT const lua::api::compilation_context* CPPLUA_GET_API_COMPILATION_CONTEXT(){
  return &__compilation_context;
}

#endif // LUA_CODE_EXISTS