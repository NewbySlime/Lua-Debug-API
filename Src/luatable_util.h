#ifndef LUATABLE_UTIL_HEADER
#define LUATABLE_UTIL_HEADER

#include "luaincludes.h"
#include "luavariant.h"
#include "macro_helper.h"

#define GLOBAL_TABLE_VARNAME "_G"


namespace lua{
  typedef void (*table_iter_callback) (const lua::api::core* lua_context, int key_stack_idx, int value_stack_idx, int iter_idx, void* cb_data);

#ifdef LUA_CODE_EXISTS

  void iterate_table(lua_State* state, int stack_idx, table_iter_callback callback, void* cb_data = NULL);
  size_t table_len(lua_State* state, int stack_idx);

  // This will create a new variant. Use cpplua_delete_variant() or I_variant_util::delete_variant() to delete it.
  variant* get_table_value(lua_State* state, int stack_idx, const variant* key);
  void set_table_value(lua_State* state, int stack_idx, const variant* key, const variant* value);

#endif // LUA_CODE_EXISTS

}

#endif