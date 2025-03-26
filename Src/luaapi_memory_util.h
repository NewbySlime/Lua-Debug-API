#ifndef LUAAPI_MEMORY_UTIL_HEADER
#define LUAAPI_MEMORY_UTIL_HEADER

#include "defines.h"
#include "library_linking.h"
#include "macro_helper.h"
#include "memdynamic_management.h"


namespace lua::api{
  class I_memory_util{
    public:
      virtual ::memory::I_dynamic_management* get_memory_manager() = 0;

      virtual const ::memory::memory_management_config* get_memory_manager_config() = 0;
      virtual void set_memory_manager_config(const ::memory::memory_management_config* config) = 0;
  };
}


#define CPPLUA_GET_API_MEMORY_UTIL_DEFINITION cpplua_get_api_memory_util_definition
#define CPPLUA_GET_API_MEMORY_UTIL_DEFINITION_STR MACRO_TO_STR_EXP(CPPLUA_GET_API_MEMORY_UTIL_DEFINITION)

typedef lua::api::I_memory_util* (__stdcall *get_api_memory_util_func)();

#ifdef LUA_CODE_EXISTS
DLLEXPORT lua::api::I_memory_util* CPPLUA_GET_API_MEMORY_UTIL_DEFINITION();
#endif // LUA_CODE_EXISTS

#endif