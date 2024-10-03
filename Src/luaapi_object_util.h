#ifndef LUAAPI_OBJECT_UTIL_HEADER
#define LUAAPI_OBJECT_UTIL_HEADER

#include "library_linking.h"
#include "luaI_object.h"
#include "macro_helper.h"


namespace lua::api{
  class I_object_util{
    public:
      virtual I_object* get_object_from_table(void* istate, int table_idx) = 0;
      virtual void push_object_to_table(void* istate, I_object* object, int table_idx) = 0;
  };
}


// MARK: DLL functions

#define CPPLUA_GET_API_OBJECT_UTIL_DEFINIITON cpplua_get_api_object_util_definition
#define CPPLUA_GET_API_OBJECT_UTIL_DEFINIITON_STR MACRO_TO_STR_EXP(CPPLUA_GET_API_OBJECT_UTIL_DEFINITION)

typedef lua::api::I_object_util* (__stdcall *get_api_object_util_func)();

#ifdef LUA_CODE_EXISTS
DLLEXPORT lua::api::I_object_util* CPPLUA_GET_API_OBJECT_UTIL_DEFINIITON();
#endif // LUA_CODE_EXISTS

#endif