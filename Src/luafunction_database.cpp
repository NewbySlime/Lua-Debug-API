#include "luafunction_database.h"
#include "stdlogger.h"

#include "cstring"

#define LUD_FUNCDB_VARNAME "__clua_function_database"


using namespace lua;
using namespace lua::debug;



// MARK: lua::func_db
func_db::func_db(lua_State* state){
  _this_state = NULL;
  _current_logger = get_stdlogger();

  func_db* _check_db = get_state_db(state);
  if(_check_db && _check_db != this){
    _current_logger->print_warning("Cannot bind func_db to lua_State. Reason: lua_State already has bound func_db.\n");
    return;
  }

  _hook_handler = hook_handler::get_this_attached(state);
  if(!_hook_handler){
    _current_logger->print_error("Cannot bind func_db to lua_State. Reason: hook_handler does not exist in current lua_State.");
    return;
  }

  _hook_handler->set_hook(LUA_MASKCALL, _hook_cb, this);

  _this_state = state;
  lightuser_var _lud_var = this;
  _lud_var.setglobal(_this_state, LUD_FUNCDB_VARNAME);
}

func_db::~func_db(){
  while(_lua_metadata_map.size() > 0){
    auto _iter = _lua_metadata_map.begin();
    _lua_delete_metadata(_iter->first);
  }

  while(_c_metadata_map.size() > 0){
    auto _iter = _c_metadata_map.begin();
    _c_delete_metadata(_iter->first);
  }

  if(_this_state){
    nil_var _nil_filler;
    _nil_filler.setglobal(_this_state, LUD_FUNCDB_VARNAME);
  }
}


func_db::_func_metadata* func_db::_lua_create_metadata(const std::string& name){
  {_func_metadata* _check_metadata = _lua_get_metadata(name);
    if(_check_metadata)
      return _check_metadata;
  }

  _func_metadata* _new_metadata = new _func_metadata();
  _lua_metadata_map[name] = _new_metadata;
  return _new_metadata;
}


func_db::_func_metadata* func_db::_lua_get_metadata(const std::string& name){
  auto _iter = _lua_metadata_map.find(name);
  return _iter != _lua_metadata_map.end()? _iter->second: NULL;
}


bool func_db::_lua_delete_metadata(const std::string& name){
  _func_metadata* _check_metadata = _lua_get_metadata(name);
  if(!_check_metadata)
    return false;

  delete _check_metadata;
  _lua_metadata_map.erase(name);
  return true;
}


func_db::_func_metadata* func_db::_c_create_metadata(const std::string& name){
  {_func_metadata* _check_metadata = _c_get_metadata(name);
    if(_check_metadata)
      return _check_metadata;
  }

  _func_metadata* _new_metadata = new _func_metadata();
  _c_metadata_map[name] = _new_metadata;
  return _new_metadata;
}

func_db::_func_metadata* func_db::_c_get_metadata(const std::string& name){
  auto _iter = _c_metadata_map.find(name);
  return _iter != _c_metadata_map.end()? _iter->second: NULL;
}

bool func_db::_c_delete_metadata(const std::string& name){
  _func_metadata* _check_metadata = _c_get_metadata(name);
  if(!_check_metadata)
    return false;

  delete _check_metadata;
  _c_metadata_map.erase(name);
  return true;
}


variant* func_db::_call_function(int arg_count, int return_count, int msgh){
  if(!_this_state)
    return NULL;

  int _error_code = lua_pcall(
    _this_state,
    arg_count,
    return_count,
    0
  );

  if(_error_code != LUA_OK){
    error_var* _err_data = new error_var(_this_state, -1);
    _err_data->set_error_code(_error_code);

    return _err_data;
  }

  if(return_count <= 0)
    return NULL;

  return to_variant(_this_state, -1);
}


void func_db::_hook_cb(lua_State* state, void* cbdata){
  func_db* _this = (func_db*)cbdata;
  const lua_Debug* _dbg = _this->_hook_handler->get_current_debug_value();
  if(_dbg->event != LUA_HOOKCALL || _dbg->name == NULL)
    return;

  _this->_current_thread_funccall[std::this_thread::get_id()] = (std::string)_dbg->name;
}


func_db* func_db::get_state_db(lua_State* state){
  variant* _lud_var = to_variant_fglobal(state, LUD_FUNCDB_VARNAME);
  func_db* _result = (func_db*)(_lud_var->get_type() == lightuser_var::get_static_lua_type()? ((lightuser_var*)_lud_var)->get_data(): NULL);

  delete _lud_var;
  return _result;
}


void func_db::set_logger(I_logger* logger){
  if(!_this_state)
    return;
    
  _current_logger = logger;
}


bool func_db::remove_lua_function_def(const char* function_name){
  if(!_this_state)
    return false;
    
  return _lua_delete_metadata(function_name);
}