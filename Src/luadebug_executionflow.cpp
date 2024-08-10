#include "luadebug_executionflow.h"
#include "luastack_iter.h"
#include "luatable_util.h"
#include "stdlogger.h"


using namespace lua;
using namespace lua::debug;


#define LUD_EXECUTION_FLOW_VARNAME "__clua_execution_flow_data"


// MARK: lua::debug::execution_flow
execution_flow::execution_flow(lua_State* state){
  _hook_handler = hook_handler::get_this_attached(state);
  _this_state = NULL;
  _logger = get_stdlogger(); 

  execution_flow* _check_obj = get_attached_obj(state);
  if(_check_obj || _check_obj != this){
    _logger->print_warning("Cannot bind execution_flow to lua_State. Reason: Another obj already bound to lua_State.");
    return;
  }

  if(!_hook_handler){
    _logger->print_error("Cannot bind execution_flow to lua_State. Reason: Cannot get bound hook_handler in lua_State.");
    return;
  }

  _this_state = state;
  _hook_handler->set_hook(LUA_MASKLINE | LUA_MASKCOUNT, _hookcb, this);

  _set_bind_obj(this, state);
}

execution_flow::~execution_flow(){
  if(_this_state)
    _set_bind_obj(NULL, _this_state);
}


void execution_flow::_set_bind_obj(execution_flow* obj, lua_State* state){
  lua_getglobal(state, LUD_EXECUTION_FLOW_VARNAME);
  if(lua_type(state, -1) != LUA_TTABLE){
    lua_pop(state, 1);

    lua_newtable(state);
    lua_setglobal(state, LUD_EXECUTION_FLOW_VARNAME);

    lua_getglobal(state, LUD_EXECUTION_FLOW_VARNAME);
  }

  string_var _key_var = from_pointer_to_str(state);
  lightuser_var _value_var = lightuser_var(obj);
  set_table_value(state, LUA_CONV_T2B(state, -1), &_key_var, &_value_var);

  // pop the global table
  lua_pop(state, 1);
}


void execution_flow::_hookcb(lua_State* state, void* cb_data){

}


execution_flow* execution_flow::get_attached_obj(lua_State* state){
  execution_flow* _result = NULL;

  lua_getglobal(state, LUD_EXECUTION_FLOW_VARNAME);
  if(lua_type(state, -1) != LUA_TTABLE)
    goto skip_checking_label;

  try{
    string_var _key_var = from_pointer_to_str(state);

    variant* _value = get_table_value(state, LUA_CONV_T2B(state, -1), &_key_var);
    if(_value->get_type() == LUA_TLIGHTUSERDATA)
      _result = (execution_flow*)((lightuser_var*)_value)->get_data();
  }
  catch(std::exception* e){
    printf(e->what());
    delete e;
  }

  skip_checking_label:{}

  lua_pop(state, 1);
  return _result;
}


void execution_flow::set_logger(I_logger* logger){
  
}