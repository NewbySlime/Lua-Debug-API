#ifndef LUAAPI_MEMORY_HEADER
#define LUAAPI_MEMORY_HEADER

#include "library_linking.h"
#include "luaincludes.h"
#include "macro_helper.h"


namespace lua::api{
  class I_memory{
    public:
      virtual int gc(void* istate, int what, int data) = 0;
      virtual lua_Alloc getallocf(void* istate, void** ud) = 0;
      virtual void* getextraspace(void* istate) = 0;
      virtual void setallocf(void* istate, lua_Alloc f, void* ud) = 0;
  };
}


#define CPPLUA_GET_API_MEMORY_DEFINITION cpplua_get_api_memory_definition
#define CPPLUA_GET_API_MEMORY_DEFINITION_STR MACRO_TO_STR_EXP(CPPLUA_GET_API_STACK_DEFINITION)

typedef lua::api::I_memory* (__stdcall *get_api_memory_func)();

#ifdef LUA_CODE_EXISTS
DLLEXPORT lua::api::I_memory* CPPLUA_GET_API_MEMORY_DEFINITION();
#endif // LUA_CODE_EXISTS

#endif