#include "luastack_iter.h"
#include "luatable_util.h"

#include "stdexcept"


void lua::iterate_table(lua_State* state, int stack_idx, table_iter_callback callback, void* cb_data){
  if(lua_type(state, stack_idx) != LUA_TTABLE)
    throw new std::runtime_error("Lua Error: Supplied stack idx is not a table.\n");

  if(!lua_checkstack(state, 3))
    throw new std::runtime_error("Lua Error: C Stack could not be resized.\n");
  
  if(stack_idx > 0)
    stack_idx = LUA_CONV_B2T(state, stack_idx);

  lua_pushnil(state);

  // NOTE: this won't work when the index is positive
  stack_idx -= 1;

  int idx = 0;
  while(lua_next(state, stack_idx)){
    int _last_stack = lua_gettop(state);
    callback(state, -2, -1, idx, cb_data);
    int _current_stack = lua_gettop(state);
    lua_pop(state, 1);

    idx++;
  }
}


// stack_idx, use bottom stack
lua::variant* lua::get_table_value(lua_State* state, int stack_idx, const lua::variant* key){
  if(lua_type(state, stack_idx) != LUA_TTABLE)
    throw new std::runtime_error("Lua Error: Supplied stack idx is not a table.\n");

  key->push_to_stack(state);
  lua_gettable(state, stack_idx);

  variant* _res_value = to_variant(state, -1);
  lua_pop(state, -1);

  return _res_value;
}

// stack_idx, use bottom stack
void lua::set_table_value(lua_State* state, int stack_idx, const variant* key, const variant* value){
  if(lua_type(state, stack_idx) != LUA_TTABLE)
    throw new std::runtime_error("Lua Error: Supplied stack idx is not a table.\n");

  // put key below value (from top)
  key->push_to_stack(state);
  value->push_to_stack(state);
  
  lua_settable(state, stack_idx);
}