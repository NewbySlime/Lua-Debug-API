#ifndef LUAAPI_TABLE_UTIL_HEADER
#define LUAAPI_TABLE_UTIL_HEADER

#include "defines.h"
#include "library_linking.h"
#include "luatable_util.h"
#include "macro_helper.h"


namespace lua::api{
  class I_table_util{
    public:
      virtual void iterate_table(void* istate, int stack_idx, lua::table_iter_callback callback, void* cb_data = NULL) = 0;
      virtual size_t table_len(void* istate, int stack_idx) = 0;
      virtual I_variant* get_table_value(void* istate, int stack_idx, const I_variant* key) = 0;
      virtual void set_table_value(void* istate, int stack_idx, const I_variant* key, const I_variant* value) = 0;
  };
}


#define CPPLUA_GET_API_TABLE_UTIL_DEFINITION cpplua_get_api_table_util_definition
#define CPPLUA_GET_API_TABLE_UTIL_DEFINITION_STR MACRO_TO_STR_EXP(CPPLUA_GET_API_TABLE_UTIL_DEFINITION)

typedef lua::api::I_table_util* (__stdcall *get_api_table_util_func)();

#ifdef LUA_CODE_EXISTS
DLLEXPORT lua::api::I_table_util* CPPLUA_GET_API_TABLE_UTIL_DEFINITION();
#endif // LUA_CODE_EXISTS

#endif