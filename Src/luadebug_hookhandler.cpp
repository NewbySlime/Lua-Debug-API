#include "lua_variant.h"
#include "luadebug_hookhandler.h"
#include "stdlogger.h"
#include "strutil.h"

#define LUD_HOOK_VAR_NAME "__clua_hook_handler"


using namespace lua;
using namespace lua::debug;

#define LUA_MAX_MASK_COUNT (LUA_HOOKCOUNT + 1)


// MARK: lua::hook_handler
hook_handler::hook_handler(lua_State* state, int count){
  _this_state = NULL;

  this->count = count;

  _logger = get_stdlogger();

  hook_handler* _this = get_this_attached(state);
  if(_this && _this != this){
    _logger->print_warning("Cannot bind hook_handler to lua_State. Reason: lua_State already has bound hook_handler.\n");
    return;
  }

  _this_state = state;
  lightuser_var _lud_var = this;
  _lud_var.setglobal(_this_state, LUD_HOOK_VAR_NAME);

  _update_hook_config();
}

hook_handler::~hook_handler(){
  if(_this_state){
    nil_var _lud_var;
    _lud_var.setglobal(_this_state, LUD_HOOK_VAR_NAME);
  }
}


void hook_handler::_update_hook_config(){
  if(!_this_state)
    return;

  lua_sethook(_this_state, _on_hook_event_static, 
    LUA_MASKCALL * (_call_hook_set.size() > 0) |
    LUA_MASKRET * (_return_hook_set.size() > 0) |
    LUA_MASKLINE * (_line_hook_set.size() > 0) |
    LUA_MASKCOUNT * (_count_hook_set.size() > 0),
  count);
}

void hook_handler::_on_hook_event(lua_State* state, lua_Debug* dbg){
  _current_dbg = dbg;
  std::map<void*, hookcb>* _current_hook_map = NULL;

  lua_getinfo(state, "nSl", _current_dbg);
  switch(dbg->event){
    break; case LUA_HOOKTAILCALL:
     case LUA_HOOKCALL:{
      _current_hook_map = &_call_hook_set;
    }

    break; case LUA_HOOKRET:{
      _current_hook_map = &_return_hook_set;
    }

    break; case LUA_HOOKLINE:{
      _current_hook_map = &_line_hook_set;
    }

    break; case LUA_HOOKCOUNT:{
      _current_hook_map = &_count_hook_set;
    }
  }

  for(auto _pair: *_current_hook_map)
    _pair.second(state, _pair.first);

  _current_dbg = NULL;
}


void hook_handler::_on_hook_event_static(lua_State* state, lua_Debug* dbg){
  hook_handler* _this = get_this_attached(state);
  if(!_this)
    return;

  _this->_on_hook_event(state, dbg);
}


DLLEXPORT hook_handler* hook_handler::get_this_attached(lua_State* state){
  variant* _lud_var = to_variant_fglobal(state, LUD_HOOK_VAR_NAME);
  hook_handler* _result = 
    (hook_handler*)(_lud_var->get_type() == lightuser_var::get_static_lua_type()? ((lightuser_var*)_lud_var)->get_data(): NULL);
  
  delete _lud_var;
  return _result;
}


void hook_handler::set_hook(int hook_mask, hook_handler::hookcb cb, void* attached_obj){
  if(!_this_state)
    return;
    
  remove_hook(attached_obj);

  for(int i = 0; i < LUA_MAX_MASK_COUNT; i++){
    int _current_mask = 1 << i;
    if(!(_current_mask & hook_mask))
      continue;

    switch(_current_mask){
      break; case LUA_MASKCALL:{
        _call_hook_set[attached_obj] = cb;
      }

      break; case LUA_MASKRET:{
        _return_hook_set[attached_obj] = cb;
      }

      break; case LUA_MASKLINE:{
        _line_hook_set[attached_obj] = cb;
      }

      break; case LUA_MASKCOUNT:{
        _count_hook_set[attached_obj] = cb;
      }
    }
  }

  _update_hook_config();
}

void hook_handler::remove_hook(void* attached_obj){
  if(!_this_state)
    return;
    
  _call_hook_set.erase(attached_obj);
  _return_hook_set.erase(attached_obj);
  _line_hook_set.erase(attached_obj);
  _count_hook_set.erase(attached_obj);

  _update_hook_config();
}


void hook_handler::set_count(int count){
  if(!_this_state)
    return;
    
  this->count = count;
  _update_hook_config();
}

int hook_handler::get_count() const{
  if(!_this_state)
    return -1;
    
  return count;
}


const lua_Debug* hook_handler::get_current_debug_value() const{
  if(!_this_state)
    return NULL;
    
  return _current_dbg;
}


void hook_handler::set_logger(I_logger* logger){
  _logger = logger;
}