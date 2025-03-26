#ifndef LUAAPI_EXECUTION_HEADER
#define LUAAPI_EXECUTION_HEADER

#include "defines.h"
#include "library_linking.h"
#include "luaincludes.h"
#include "macro_helper.h"


namespace lua::api{
  class I_execution{
    public:
      virtual void call(void* istate, int nargs, int nresults) = 0;
      virtual void callk(void* istate, int nargs, int nresults, lua_KContext ctx, lua_KFunction k) = 0;
      virtual int pcall(void* istate, int nargs, int nresults, int msgh) = 0;
      virtual int pcallk(void* istate, int nargs, int nresults, int msgh, lua_KContext ctx, lua_KFunction k) = 0;
      virtual int resume(void* istate, void* ifrom, int nargs) = 0;
      virtual int status(void* istate) = 0;
      virtual int yield(void* istate, int nresults);
      virtual int yieldk(void* istate, int nresults, lua_KContext ctx, lua_KFunction k) = 0;
  };
}


#define CPPLUA_GET_API_EXECUTION_DEFINITION cpplua_get_api_execution_defintion
#define CPPLUA_GET_API_EXECUTION_DEFINITION_STR MACRO_TO_STR_EXP(CPPLUA_GET_API_EXECUTION_DEFINITION)

typedef lua::api::I_execution* (__stdcall *get_api_execution_func)();

#ifdef LUA_CODE_EXISTS
DLLEXPORT lua::api::I_execution* CPPLUA_GET_API_EXECUTION_DEFINITION();
#endif // LUA_CODE_EXISTS

#endif