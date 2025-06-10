#ifndef LUAAPI_INTERNAL_HEADER
#define LUAAPI_INTERNAL_HEADER

#include "defines.h"
#include "library_linking.h"
#include "luaapi_core.h"
#include "macro_helper.h"


namespace lua::api{
  class I_internal{
    public:
      virtual void initiate_internal_storage(void* istate) = 0;
      virtual void require_internal_storage(void* istate) = 0;
  };
}


#if (__linux)
// Just to make the compiler happy
#define __stdcall 
#endif


#define CPPLUA_GET_API_INTERNAL_DEFINITION cpplua_get_api_internal_definition
#define CPPLUA_GET_API_INTERNAL_DEFINITION_STR MACRO_TO_STR_EXP(CPPLUA_GET_API_INTERNAL_DEFINITION)

typedef const lua::api::I_internal* (__stdcall *get_api_internal_definition)();

#ifdef LUA_CODE_EXISTS
DLLEXPORT lua::api::I_internal* CPPLUA_GET_API_INTERNAL_DEFINITION();
#endif // LUA_CODE_EXISTS

#endif