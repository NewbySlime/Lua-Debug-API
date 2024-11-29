#include "luaapi_stack.h"
#include "luaapi_value.h"
#include "luaapi_variant_util.h"
#include "luamemory_util.h"
#include "luastack_iter.h"
#include "luautility.h"
#include "luavariant_util.h"
#include "luavariant.h"
#include "string_util.h"

using namespace lua;
using namespace lua::api;
using namespace lua::memory;
using namespace lua::utility;
using namespace ::memory;


#define VARIANT_SPECIAL_TYPE_KEY_REG "__clua_special_type"


static const I_dynamic_management* __dm = get_memory_manager();


// MARK: to_variant

variant* lua::to_variant(const core* lc, int stack_idx){
  // since from_state functionality uses absindex, the value will be copied first
  variant* _result = NULL;;
  lc->context->api_stack->pushvalue(lc->istate, stack_idx);
  stack_idx = -1;

  int _type = lc->context->api_varutil->get_special_type(lc->istate, stack_idx);
  switch(_type){
    break; case number_var::get_static_lua_type():{
      _result = __dm->new_class_dbg<number_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, lc, stack_idx);
    }

    break; case string_var::get_static_lua_type():{
      _result = __dm->new_class_dbg<string_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, lc, stack_idx);
    }

    break; case bool_var::get_static_lua_type():{
      _result = __dm->new_class_dbg<bool_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, lc, stack_idx);
    }

    break; case table_var::get_static_lua_type():{
      _result = __dm->new_class_dbg<table_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, lc, stack_idx);
    }

    break; case lightuser_var::get_static_lua_type():{
      _result = __dm->new_class_dbg<lightuser_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, lc, stack_idx);
    }

    break; case function_var::get_static_lua_type():{
      _result = __dm->new_class_dbg<function_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, lc, stack_idx);
    }

    break; case object_var::get_static_lua_type():{
      _result = __dm->new_class_dbg<object_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, lc, stack_idx);
    }

    break; default:{
      _result = __dm->new_class_dbg<variant>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
    }
  }

  lc->context->api_stack->pop(lc->istate, 1);
  return _result;
}

variant* lua::to_variant_fglobal(const core* lc, const char* global_name){
  lc->context->api_value->getglobal(lc->istate, global_name);
  variant* _result = to_variant(lc, -1);
  lc->context->api_stack->pop(lc->istate, 1);

  return _result;
}

variant* lua::to_variant_ref(const core* lc, int stack_idx){
  const void* _pointer_ref = lc->context->api_value->topointer(lc->istate, stack_idx);
  string_var* _var_res = __dm->new_class_dbg<string_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, format_str_mem(__dm, "p_ref 0x%X", _pointer_ref));

  return _var_res;
}


string_var lua::from_pointer_to_str(void* pointer){
  long long _pointer_num = (long long)pointer;
  return string_var(reinterpret_cast<const char*>(_pointer_num), sizeof(long long));
}

void* lua::from_str_to_pointer(const string_var& str){
  long long _pointer_num = *(reinterpret_cast<const long long*>(str.get_string()));
  return (void*)_pointer_num;
}


#ifdef LUA_CODE_EXISTS

variant* lua::to_variant(lua_State* state, int stack_idx){
  core _lc = as_lua_api_core(state);
  return to_variant(&_lc, stack_idx);
}

variant* lua::to_variant_fglobal(lua_State* state, const char* global_name){
  core _lc = as_lua_api_core(state);
  return to_variant_fglobal(&_lc, global_name);
}

variant* lua::to_variant_ref(lua_State* state, int stack_idx){
  core _lc = as_lua_api_core(state);
  return to_variant_ref(&_lc, stack_idx);
}


std::string lua::to_string(lua_State* state, int stack_idx){
  variant* _tmpvar = to_variant(state, stack_idx);
  std::string _result = _tmpvar->to_string();
  cpplua_delete_variant(_tmpvar);

  return _result;
}


void lua::set_special_type(lua_State* state, int stack_idx, int new_type){
  stack_idx = stack_idx < 0? LUA_CONV_T2B(state, stack_idx): stack_idx;
  if(lua_type(state, stack_idx) == LUA_TNIL)
    return;

  if(!lua_getmetatable(state, stack_idx)){
    lua_pop(state, 1); // pop the nil value first

    lua_newtable(state);
    lua_pushvalue(state, -1); // for later use
    lua_setmetatable(state, stack_idx);
  }

  lua_pushstring(state, VARIANT_SPECIAL_TYPE_KEY_REG);
  lua_pushinteger(state, new_type);
  lua_settable(state, -3); // set data in metatable

  // pop the metatable
  lua_pop(state, 1);
}

int lua::get_special_type(lua_State* state, int stack_idx){
  return pstack_call_context<int>(state, 0, 0, cpplua_get_api_compilation_context(), [](lua_State* state, int stack_idx){
    stack_idx = stack_idx < 0? LUA_CONV_T2B(state, stack_idx): stack_idx;
    int _ltype = lua_type(state, stack_idx);
    if(_ltype == LUA_TNIL)
      return LUA_TNIL;

    if(!lua_getmetatable(state, stack_idx))
      return _ltype;

    lua_pushstring(state, VARIANT_SPECIAL_TYPE_KEY_REG);
    lua_gettable(state, -2);
    if(lua_type(state, -1) == LUA_TNIL)
      return _ltype;

    int _result = lua_tointeger(state, -1);
    return _result; // stack stabilized by pstack_call()
  }, state, stack_idx);
}


void lua::set_global(lua_State* state, const char* global_name, I_variant* var){
  core _lc = as_lua_api_core(state);
  var->push_to_stack(&_lc);
  lua_setglobal(state, global_name);
}

#endif // LUA_CODE_EXISTS