#ifndef LUAVARIANT_UTIL_HEADER
#define LUAVARIANT_UTIL_HEADER

#include "luavariant.h"


namespace lua{
  // NOTE: Reason for to_variant are included also in the static library so that any object use same stdlib memory management functions as the compiled program.

  // This will not pop the value.
  // WARN: Do not forget to delete returned object using cpplua_delete_variant() or I_variant_util::delete_variant().
  variant* to_variant(const lua::api::core* lua_core, int stack_idx);
  // This will not pop the value.
  // WARN: Do not forget to delete returned object using cpplua_delete_variant() or I_variant_util::delete_variant().
  variant* to_variant_fglobal(const lua::api::core* lua_core, const char* global_name);
  // Pointer value (in lua) parsed to string variant.
  // WARN: Do not forget to delete returned object using cpplua_delete_variant() or I_variant_util::delete_variant().
  variant* to_variant_ref(const lua::api::core* lua_core, int stack_idx);

  string_var from_pointer_to_str(void* pointer);
  void* from_str_to_pointer(const string_var& str);

#ifdef LUA_CODE_EXISTS
  // This will not pop the value.
  // WARN: Do not forget to delete returned object using cpplua_delete_variant() or I_variant_util::delete_variant().
  variant* to_variant(lua_State* state, int stack_idx);
  // Get variant from determined global_value.
  // WARN: Do not forget to delete returned object using cpplua_delete_variant() or I_variant_util::delete_variant().
  variant* to_variant_fglobal(lua_State* state, const char* global_name);
  // Pointer value (in lua) parsed to string variant.
  // WARN: Do not forget to delete returned object using cpplua_delete_variant() or I_variant_util::delete_variant().
  variant* to_variant_ref(lua_State* state, int stack_idx);

  std::string to_string(lua_State* state, int stack_idx);

  // Automatically create metatable if not yet exists.
  // The function will ignore values that is a LUA_TNIL type.
  void set_special_type(lua_State* state, int stack_idx, int new_type);
  // Returns LUA_TNIL if the target value is nil.
  // Returns the original (Lua type) of the value if it does not have the special type or not yet set.
  int get_special_type(lua_State* state, int stack_idx);

  void set_global(lua_State* state, const char* global_name, I_variant* var);
#endif // LUA_CODE_EXISTS
}

#endif