#include "luaapi_compilation_context.h"
#include "luaapi_debug.h"
#include "luaapi_execution.h"
#include "luaapi_internal.h"
#include "luaapi_memory.h"
#include "luaapi_memory_util.h"
#include "luaapi_object_util.h"
#include "luaapi_runtime.h"
#include "luaapi_stack.h"
#include "luaapi_state.h"
#include "luaapi_thread.h"
#include "luaapi_util.h"
#include "luaapi_value.h"
#include "luaapi_table_util.h"
#include "luaapi_variant_util.h"


#ifdef LUA_CODE_EXISTS

using namespace lua::api;



compilation_context::compilation_context(I_debug* debug, I_execution* execution, I_internal* internal, I_memory* memory, I_memory_util* memory_util, I_object_util* objutil, I_runtime* runtime, I_stack* stack, I_state* state, I_thread* thread_api, I_util* util, I_value* value, I_table_util* tableutil, I_variant_util* varutil){
  api_debug = debug;
  api_execution = execution;
  api_internal = internal;
  api_memory = memory;
  api_memutil = memory_util;
  api_objutil = objutil;
  api_runtime = runtime;
  api_stack = stack;
  api_state = state;
  api_thread = thread_api;
  api_util = util;
  api_value = value;
  api_tableutil = tableutil;
  api_varutil = varutil;
}


static const lua::api::compilation_context __compilation_context(
  CPPLUA_GET_API_DEBUG_DEFINITION(),
  CPPLUA_GET_API_EXECUTION_DEFINITION(),
  CPPLUA_GET_API_INTERNAL_DEFINITION(),
  CPPLUA_GET_API_MEMORY_DEFINITION(),
  CPPLUA_GET_API_MEMORY_UTIL_DEFINITION(),
  CPPLUA_GET_API_OBJECT_UTIL_DEFINITION(),
  CPPLUA_GET_API_RUNTIME_DEFINITION(),
  CPPLUA_GET_API_STACK_DEFINITION(),
  CPPLUA_GET_API_STATE_DEFINITION(),
  CPPLUA_GET_API_THREAD_DEFINITION(),
  CPPLUA_GET_API_UTIL_DEFINITION(),
  CPPLUA_GET_API_VALUE_DEFINITION(),
  CPPLUA_GET_API_TABLE_UTIL_DEFINITION(),
  CPPLUA_GET_API_VARIANT_UTIL_DEFINITION()
);

DLLEXPORT const lua::api::compilation_context* CPPLUA_GET_API_COMPILATION_CONTEXT(){
  return &__compilation_context;
}

#endif // LUA_CODE_EXISTS