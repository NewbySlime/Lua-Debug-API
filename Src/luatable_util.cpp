#include "luastack_iter.h"
#include "luatable_util.h"
#include "luautility.h"
#include "luavariant_util.h"

#include "stdexcept"

using namespace lua::api;
using namespace lua::utility;


#ifdef LUA_CODE_EXISTS

void lua::iterate_table(lua_State* state, int stack_idx, table_iter_callback callback, void* cb_data){
  // Convert to top aligned stack
  stack_idx = stack_idx > 0? LUA_CONV_B2T(state, stack_idx): stack_idx;

  if(lua_type(state, stack_idx) != LUA_TTABLE)
    throw new std::runtime_error("Lua Error: Supplied stack idx is not a table.\n");

  if(!lua_checkstack(state, 3))
    throw new std::runtime_error("Lua Error: C Stack could not be resized.\n");

  lua_pushnil(state);
  stack_idx -= 1; // Offset index to table variable

  // For callback
  core _core = as_lua_api_core(state);

  int idx = 0;
  while(lua_next(state, stack_idx)){
    int _last_stack = lua_gettop(state);
    callback(&_core, -2, -1, idx, cb_data);
    int _current_stack = lua_gettop(state);
    lua_pop(state, 1);

    idx++;
  }
}


size_t lua::table_len(lua_State* state, int stack_idx){
  size_t _result = 0;
  iterate_table(state, stack_idx, [](const lua::api::core*, int, int, int, void* _len){
    (*(size_t*)_len)++;
  }, &_result);

  return _result;
}


// stack_idx, use bottom stack
lua::variant* lua::get_table_value(lua_State* state, int stack_idx, const lua::variant* key){
  core _lc = as_lua_api_core(state);

  // Convert to bottom aligned stack
  stack_idx = stack_idx < 0? LUA_CONV_T2B(state, stack_idx): stack_idx;

  if(lua_type(state, stack_idx) != LUA_TTABLE)
    throw new std::runtime_error("Lua Error: Supplied stack idx is not a table.\n");

  key->push_to_stack(&_lc);
  lua_gettable(state, stack_idx);

  variant* _res_value = to_variant(state, -1);
  lua_pop(state, -1);

  return _res_value;
}

// stack_idx, use bottom stack
void lua::set_table_value(lua_State* state, int stack_idx, const variant* key, const variant* value){
  core _lc = as_lua_api_core(state);

  // Convert to bottom aligned sack
  stack_idx = stack_idx < 0? LUA_CONV_T2B(state, stack_idx): stack_idx;
  if(lua_type(state, stack_idx) != LUA_TTABLE)
    throw new std::runtime_error("Lua Error: Supplied stack idx is not a table.\n");

  // put key below value (from top)
  key->push_to_stack(&_lc);
  value->push_to_stack(&_lc);
  
  lua_settable(state, stack_idx);
}

#endif // LUA_CODE_EXISTS