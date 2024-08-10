#ifndef LUATABLE_UTIL_HEADER
#define LUATABLE_UTIL_HEADER

#include "lua_includes.h"
#include "lua_variant.h"

#define GLOBAL_TABLE_VARNAME "_G"


namespace lua{
  typedef void (*table_iter_callback) (lua_State* state, int key_stack_idx, int value_stack_idx, int iter_idx, void* cb_data);

  void iterate_table(lua_State* state, int stack_idx, table_iter_callback callback, void* cb_data = NULL);


  // only accept key in string, boolean and number
  variant* get_table_value(lua_State* state, int stack_idx, const variant* key);

  // only accept key in string, boolean and number
  void set_table_value(lua_State* state, int stack_idx, const variant* key, const variant* value);
}


#endif