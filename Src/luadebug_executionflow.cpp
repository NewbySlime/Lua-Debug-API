#include "luadebug_executionflow.h"
#include "luastack_iter.h"
#include "luatable_util.h"
#include "stdlogger.h"


using namespace lua;
using namespace lua::debug;


#define LUD_EXECUTION_FLOW_VARNAME "__clua_execution_flow_data"

#define FNAME_TAILCALLED_FLAG " (tailcalled)"


// MARK: lua::debug::execution_flow
execution_flow::execution_flow(lua_State* state){
  _hook_handler = hook_handler::get_this_attached(state);
  _this_state = NULL;
  _logger = get_stdlogger();

  if(!state){
    _logger->print_error("Cannot bind execution_flow to lua_State. Reason: lua_State provided is NULL.\n");
    return;
  }

  execution_flow* _check_obj = get_attached_obj(state);
  if(_check_obj){
    _logger->print_warning("Cannot bind execution_flow to lua_State. Reason: Another obj already bound to lua_State.\n");
    return;
  }

  if(!_hook_handler){
    _logger->print_error("Cannot bind execution_flow to lua_State. Reason: Cannot get bound hook_handler in lua_State.\n");
    return;
  }

  _this_state = state;
  _hook_handler->set_hook(LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE | LUA_MASKCOUNT, _hookcb_static, this);

  _set_bind_obj(this, state);
}

execution_flow::~execution_flow(){
  if(!_this_state)
    return;

  _hook_handler->remove_hook(this);
  _set_bind_obj(NULL, _this_state);

  while(_function_layer.size() > 0){
    auto _iter = _function_layer.end()-1;
    _function_data* _data = *_iter;
    
    _function_layer.erase(_iter);
    delete _data;
  }

  if(currently_pausing())
    resume_execution();
}


void execution_flow::_block_execution(){
  std::unique_lock<std::mutex> _ulock(_execution_mutex);

  _currently_executing = false;
  _execution_block.wait(_ulock);
  _currently_executing = true;
}


void execution_flow::_hookcb(){
  _currently_executing = true;
  const lua_Debug* _debug_data = _hook_handler->get_current_debug_value();

  switch(_debug_data->event){
    // Debug info does not track function name when it comes to tailcalling
    break; case LUA_HOOKTAILCALL:{
      if(_function_layer.size() <= 0)
        break;

      _function_data* _data = *(_function_layer.end()-1);
      _data->is_tailcall = true;
      _data->name += FNAME_TAILCALLED_FLAG;
    }

    break; case LUA_HOOKCALL:{
      _function_data* _new_data = new _function_data();
      _new_data->name = _debug_data->name? _debug_data->name: "";
      _new_data->is_tailcall = false;

      _function_layer.insert(_function_layer.end(), _new_data);
    }

    break; case LUA_HOOKRET:{
      if(_function_layer.size() <= 0)
        break;

      auto _iter = _function_layer.end()-1;
      _function_data* _data = *_iter;

      _function_layer.erase(_iter);
      delete _data;
    }
  }

  if(!_do_block)
    return;

  if(_stepping_type != st_none){
    switch(_debug_data->event){
      break; case LUA_HOOKCOUNT:{
        if(_stepping_type != step_type::st_per_counts)
          return;
      }

      break; case LUA_HOOKLINE:{
        int _layer_count = get_function_layer();
        if(
          _stepping_type != step_type::st_per_line &&
          (_stepping_type != step_type::st_over || _layer_count != _step_layer_check) &&
          _stepping_type != step_type::st_in &&
          (_stepping_type != step_type::st_out || _layer_count > _step_layer_check)
        )
          return;
      }

      break; case LUA_HOOKTAILCALL:
       case LUA_HOOKCALL:{
        if(_stepping_type != step_type::st_in)
          return;
      }

      break; case LUA_HOOKRET:{
        int _layer_count = get_function_layer();
        if(
          (_stepping_type != step_type::st_out || _layer_count > _step_layer_check) &&
          (_stepping_type != step_type::st_over || _layer_count > _step_layer_check)
        )
          return;
      }
    }
  }

  _block_execution();
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

  // possible multiple object in one state for in case of multi threading
  set_table_value(state, LUA_CONV_T2B(state, -1), &_key_var, &_value_var);

  // pop the global table
  lua_pop(state, 1);
}


void execution_flow::_hookcb_static(lua_State* state, void* cb_data){
  ((execution_flow*)cb_data)->_hookcb();
}


DLLEXPORT execution_flow* execution_flow::get_attached_obj(lua_State* state){
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


void execution_flow::set_step_count(int count){
  if(!_this_state)
    return;

  _hook_handler->set_count(count);
}

int execution_flow::get_step_count() const{
  if(!_this_state)
    return -1;

  return _hook_handler->get_count();
}


unsigned int execution_flow::get_function_layer() const{
  return _function_layer.size();
}


const char* execution_flow::get_function_name() const{
  if(_function_layer.size() <= 0)
    return "";

  _function_data* _data = (*(_function_layer.end()-1));
  
  // Debug info does not track function name when it comes to tailcalling
  return _data->name.c_str();
}


bool execution_flow::set_function_name(int layer_idx, const char* name){
  return set_function_name(layer_idx, std::string(name));
}

bool execution_flow::set_function_name(int layer_idx, const std::string& name){
  if(layer_idx < 0 || layer_idx >= _function_layer.size())
    return false;

  _function_data* _data = _function_layer[layer_idx];

  _data->name = name + (_data->is_tailcall? FNAME_TAILCALLED_FLAG: "");
  return true;
}


void execution_flow::block_execution(){
  if(!_this_state)
    return;
    
  _do_block = true;
  _stepping_type = st_none;
}

void execution_flow::resume_execution(){
  if(!_this_state)
    return;
    
  _do_block = false;
  _execution_block.notify_all();
}

void execution_flow::step_execution(step_type st){
  if(!_this_state)
    return;
    
  block_execution();

  _stepping_type = st;
  switch(st){
    break; case step_type::st_over:{
      _step_layer_check = get_function_layer();
    }

    break; case step_type::st_in:{
      _step_layer_check = get_function_layer()+1;
    }

    break; case step_type::st_out:{
      _step_layer_check = get_function_layer()-1;
    }
  }

  _execution_block.notify_all();
}


bool execution_flow::currently_pausing(){
  return !_currently_executing;
}


const lua_Debug* execution_flow::get_debug_data() const{
  if(!_this_state)
    return NULL;
    
  return _hook_handler->get_current_debug_value();
}


void execution_flow::set_logger(I_logger* logger){
  if(!_this_state)
    return;
    
  _logger = logger;
}