#include "luaapi_core.h"
#include "luacpp_error_handling.h"
#include "luamemory_util.h"
#include "luaobject_util.h"
#include "luastack_iter.h"
#include "luautility.h"
#include "luavariant.h"
#include "luavariant_arr.h"
#include "luavariant_util.h"

#define OBJECT_VAR_REF_VALUE_NAME "_cpp_obj_ref"


using namespace lua;
using namespace lua::api;
using namespace lua::memory;
using namespace lua::object;
using namespace lua::utility;
using namespace ::memory;


// MARK: function_store definition

static const I_dynamic_management* __dm = get_memory_manager();


function_store::function_store(object_destructor_func destructor){
  _deinit_func = destructor;
  set_function_data(NULL);
}


void function_store::set_function_data(const fdata* list){
  _function_list = list;
  _func_count = 0;
  if(!_function_list)
    return;

  bool _do_loop = true;
  while(_do_loop){
    _do_loop = _function_list[_func_count].func != NULL;
    _func_count++;
  }

  _func_count--;
}


void function_store::set_destructor(object_destructor_func destructor){
  _deinit_func = destructor;
}


I_object::object_destructor_func function_store::get_this_destructor() const{
  return _deinit_func;
}


int function_store::get_function_count() const{
  return _func_count;
}

const char* function_store::get_function_name(int idx) const{
  if(idx < 0 || idx >= _func_count)
    return NULL;

  return _function_list[idx].name;
}

I_object::lua_function function_store::get_function(int idx) const{
  if(idx < 0 || idx >= _func_count)
    return NULL;

  return _function_list[idx].func;
}



// MARK: lua::object functions

#ifdef LUA_CODE_EXISTS

static int _on_obj_called(lua_State* state){
  return protected_callcpp<int, 0>(state, [](lua_State* state){
    core _lc = as_lua_api_core(state);

    I_object* _obj = get_object_from_table(state, 1);
    if(!_obj){
      string_var _err_msg = "[CPPLua] Cannot call function, object already deinstantiated.";
      throw __dm->new_class_dbg<error_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, &_err_msg, -1);
    }

    int _func_idx = lua_tointeger(state, lua_upvalueindex(1));
    I_object::lua_function _lf = _obj->get_function(_func_idx);
    if(!_lf){
      string_var _err_msg = "[CPPLua] Cannot call function, object does not have the intended function.";
      throw __dm->new_class_dbg<error_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, &_err_msg, -1);
    }

    vararr _result_arr, _arg_arr;

    // fetch argument values
    int _arg_count = lua_gettop(state);
    for(int i = 1; i < _arg_count; i++){
      variant* _val = to_variant(state, i+1);
      _arg_arr.append_var(_val);

      cpplua_delete_variant(_val);
    }
    
    _lf(_obj, &_arg_arr, &_result_arr);

    const I_variant* _check_var = _result_arr.get_var(0);
    if(_check_var && _check_var->get_type() == error_var::get_static_lua_type()){
      variant* _err_val = cpplua_create_var_copy(_check_var);
      throw (error_var*)_err_val;
    }

    // push results
    for(int i = 0; i < _result_arr.get_var_count(); i++){
      variant* _val = cpplua_create_var_copy(_result_arr.get_var(i));
      _val->push_to_stack(&_lc);
      cpplua_delete_variant(_val);
    }

    return _result_arr.get_var_count();
  }, state);
}

static int _on_obj_removed(lua_State* state){
  return protected_callcpp<int, 0>(state, [](lua_State* state){
    I_object* _obj = get_object_from_table(state, 1);
    if(!_obj){
      lua_getglobal(state, "print");
      lua_pushstring(state, "[WARN][CPPLua] Object already deleted.");
      lua_pcall(state, 1, 0, 0);

      return 0;
    }

    lua_getmetatable(state, 1);
    
    lua_pushstring(state, OBJECT_VAR_REF_VALUE_NAME);
    lua_pushlightuserdata(state, NULL);
    lua_rawset(state, -3);

    lua_pop(state, 1);

    _obj->get_this_destructor()(_obj);

    return 0;
  }, state);
}


// index based from the bottom
I_object* lua::object::get_object_from_table(lua_State* state, int table_idx){
  if(table_idx < 0)
    table_idx = LUA_CONV_T2B(state, table_idx);

  if(lua_type(state, table_idx) != LUA_TTABLE)
    return NULL;

  int _prev_stack = lua_gettop(state);
  int _check, _type;

  I_object* _result = NULL;

  lua_getmetatable(state, table_idx);

  lua_pushstring(state, OBJECT_VAR_REF_VALUE_NAME);
  _type = lua_rawget(state, -2);
  if(_type != LUA_TLIGHTUSERDATA)
    goto on_skip_label;

  _result = (I_object*)lua_touserdata(state, -1);
  lua_pop(state, 2);

  on_skip_label:{}

  int _stack_delta = lua_gettop(state) - _prev_stack;
  if(_stack_delta > 0)
    lua_pop(state, _stack_delta);

  return _result;
}


void lua::object::push_object_to_table(lua_State* state, I_object* object, int table_idx){
  core _lc = as_lua_api_core(state);

  if(table_idx < 0) 
    table_idx = LUA_CONV_T2B(state, table_idx);

  // object's metatable
  lua_newtable(state);

  // push garbage collector function
  lua_pushstring(state, "__gc");
  lua_pushcfunction(state, _on_obj_removed);
  lua_rawset(state, -3);

  // push obj
  lua_pushstring(state, OBJECT_VAR_REF_VALUE_NAME);
  lua_pushlightuserdata(state, object);
  lua_rawset(state, -3);

  // set object's metatable
  lua_setmetatable(state, -2);

  // push functions
  for(int i = 0; i < object->get_function_count(); i++){
    // fname
    lua_pushstring(state, object->get_function_name(i));

    // c closure
    lua_pushinteger(state, i);
    lua_pushcclosure(state, _on_obj_called, 1);
    
    lua_settable(state, -3);
  }

  lua_pushvalue(state, table_idx);
  object->on_object_added(&_lc);
  lua_pop(state, 1);
}

#endif // LUA_CODE_EXISTS