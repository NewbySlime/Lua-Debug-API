#ifndef LUATABLE_UTIL_HEADER
#define LUATABLE_UTIL_HEADER

#include "lua_includes.h"
#include "lua_variant.h"
#include "macro_helper.h"

#define GLOBAL_TABLE_VARNAME "_G"


namespace lua{
  typedef void (*table_iter_callback) (lua_State* state, int key_stack_idx, int value_stack_idx, int iter_idx, void* cb_data);

  void iterate_table(lua_State* state, int stack_idx, table_iter_callback callback, void* cb_data = NULL);
  size_t table_len(lua_State* state, int stack_idx);


  // only accept key in string, boolean and number
  variant* get_table_value(lua_State* state, int stack_idx, const variant* key);

  // only accept key in string, boolean and number
  void set_table_value(lua_State* state, int stack_idx, const variant* key, const variant* value);

  
  namespace api{
    class I_table_util{
      public:
        virtual void iterate_table(void* istate, int stack_idx, table_iter_callback callback, void* cb_data = NULL) = 0;
        virtual size_t table_len(void* istate, int stack_idx) = 0;
        virtual I_variant* get_table_value(void* istate, int stack_idx, const I_variant* key) = 0;
        virtual void set_table_value(void* istate, int stack_idx, const I_variant* key, const I_variant* value) = 0;
    };
  }
}


#define CPPLUA_GET_API_TABLE_UTIL_DEFINITION cpplua_get_api_table_util_definition
#define CPPLUA_GET_API_TABLE_UTIL_DEFINITION_STR MACRO_TO_STR_EXP(CPPLUA_GET_API_STACK_DEFINITION)

typedef lua::api::I_table_util* (__stdcall *get_api_table_util_func)();

DLLEXPORT lua::api::I_table_util* CPPLUA_GET_API_TABLE_UTIL_DEFINITION();

#endif