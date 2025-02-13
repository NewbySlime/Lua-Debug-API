#ifndef LUAAPI_COMPILATION_CONTEXT_HEADER
#define LUAAPI_COMPILATION_CONTEXT_HEADER

#include "defines.h"
#include "library_linking.h"
#include "luaincludes.h"


namespace lua::api{
  class I_debug;
  class I_execution;
  class I_internal;
  class I_memory;
  class I_memory_util;
  class I_object_util;
  class I_runtime;
  class I_stack;
  class I_state;
  class I_thread;
  class I_util;
  class I_value;
  class I_table_util;
  class I_variant_util;
  
  struct compilation_context{
#ifdef LUA_CODE_EXISTS
    compilation_context(I_debug* debug, I_execution* execution, I_internal* internal, I_memory* memory, I_memory_util* memory_util, I_object_util* objutil, I_runtime* runtime, I_stack* stack, I_state* state, I_thread* thread_api, I_util* util, I_value* value, I_table_util* tableutil, I_variant_util* varutil);
#endif // LUA_CODE_EXISTS

    I_debug* api_debug;
    I_execution* api_execution;
    I_internal* api_internal;
    I_memory* api_memory;
    I_memory_util* api_memutil;
    I_object_util* api_objutil;
    I_runtime* api_runtime;
    I_stack* api_stack;
    I_state* api_state;
    I_thread* api_thread;
    I_util* api_util;
    I_value* api_value;
    I_table_util* api_tableutil;
    I_variant_util* api_varutil;
  };
}


#define CPPLUA_GET_API_COMPILATION_CONTEXT cpplua_get_api_compilation_context
#define CPPLUA_GET_API_COMPILATION_CONTEXT_STR MACRO_TO_STR_EXP(cpplua_get_api_compilation_context)

typedef const lua::api::compilation_context* (__stdcall *get_api_compilation_context)();

#ifdef LUA_CODE_EXISTS
DLLEXPORT const lua::api::compilation_context* CPPLUA_GET_API_COMPILATION_CONTEXT();
#endif // LUA_CODE_EXISTS

#endif