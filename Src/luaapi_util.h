#ifndef LUAAPI_UTIL_HEADER
#define LUAAPI_UTIL_HEADER

#include "defines.h"
#include "library_linking.h"
#include "luageneral_usage_runtime.h"
#include "luaincludes.h"
#include "macro_helper.h"


namespace lua::api{
  class I_util{
    public:
      virtual int absindex(void* istate, int idx) = 0;
      virtual lua_CFunction atpanic(void* istate, lua_CFunction panicf) = 0;
      virtual int dump(void* istate, lua_Writer writer, void* data, int strip) = 0;
      virtual int error(void* istate) = 0;
      virtual const lua_Number* version(void* istate) = 0;

      virtual void* get_main_thread(void* istate) = 0;
      virtual void* require_general_usage_runtime() = 0;
      virtual lua::api::core require_general_usage_core() = 0;
  };
}


#if (__linux)
// Just to make the compiler happy
#define __stdcall 
#endif


#define CPPLUA_GET_API_UTIL_DEFINITION cpplua_get_api_util_definition
#define CPPLUA_GET_API_UTIL_DEFINITION_STR MACRO_TO_STR_EXP(CPPLUA_GET_API_UTIL_DEFINITION)

typedef lua::api::I_util* (__stdcall *get_api_util_func)();

#ifdef LUA_CODE_EXISTS
DLLEXPORT lua::api::I_util* CPPLUA_GET_API_UTIL_DEFINITION();
#endif // LUA_CODE_EXISTS

#endif