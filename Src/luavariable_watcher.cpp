#include "library_linking.h"
#include "luatable_util.h"
#include "luavariable_watcher.h"
#include "stdlogger.h"

#define LUD_VARIABLE_WATCHER_VAR_NAME "__clua_variable_watcher"


using namespace lua;
using namespace lua::debug;


// MARK: lua::debug::variable_watcher

variable_watcher::variable_watcher(lua_State* state){
  _logger = get_stdlogger();
  _state = NULL;

  // check if already exists
  {variable_watcher* _obj = get_attached_object(state);
    if(_obj){
      _logger->print_error("[variable_watcher] Cannot initialize. Reason: lua_State already has variable_watcher.\n");
      return;
    }
  }

  _state = state;
}

variable_watcher::~variable_watcher(){
  _clear_variable_data();
}


void variable_watcher::_fetch_function_variable_data(lua_Debug* debug_data){
  _clear_variable_data();

  int i = 0;
  while(true){
    const char* _var_name_str = lua_getlocal(_state, debug_data, i+1);
    if(!_var_name_str)
      break;

    _variable_data* _vdata = new _variable_data();
    _vdata->var_name = _var_name_str;
    _vdata->var_data = to_variant(_state, -1);
    _vdata->lua_type = lua_type(_state, -1);

    lua_pop(_state, 1);

    _vdata_list.insert(_vdata_list.end(), _vdata);
    _vdata_map[_var_name_str] = _vdata;

    i++;
  }
}

void variable_watcher::_clear_variable_data(){
  for(_variable_data* _data: _vdata_list){
    cpplua_delete_variant(_data->var_data);
    delete _data;
  }

  _vdata_list.clear();
  _vdata_map.clear();
}


void variable_watcher::_set_bind_obj(variable_watcher* obj, lua_State* state){
  lightuser_var _lud_var = obj;
  _lud_var.setglobal(state, LUD_VARIABLE_WATCHER_VAR_NAME);
}


variable_watcher* variable_watcher::get_attached_object(lua_State* state){
  variant* _lud_var = to_variant_fglobal(state, LUD_VARIABLE_WATCHER_VAR_NAME);
  variable_watcher* _result =
    (variable_watcher*)(_lud_var->get_type() == lightuser_var::get_static_lua_type()?((lightuser_var*)_lud_var)->get_data(): NULL);

  cpplua_delete_variant(_lud_var);
  return _result;
}


bool variable_watcher::fetch_current_function_variables(){
  if(!_state)
    return false;

  bool _result = true;
  lua_Debug* _debug_data = NULL;

  _debug_data = (lua_Debug*)calloc(1, sizeof(lua_Debug)); 
  if(!lua_getstack(_state, 0, _debug_data)){
    if(_logger)
      _logger->print_error("[variable_watcher] Cannot get current function stack info.\n");

    _result = false;
    goto cleanup_label;
  }

  _fetch_function_variable_data(_debug_data);

  cleanup_label:{
    free(_debug_data);
  }

  return _result;
}


bool variable_watcher::fetch_global_table_data(){
  _clear_variable_data();

  lua_pushglobaltable(_state);

  iterate_table(_state, -1, [](lua_State* state, int key_stack_idx, int value_stack_idx, int iter_idx, void* cb_data){
    variable_watcher* _this = (variable_watcher*)cb_data;

    variant* _key_data = to_variant(state, key_stack_idx);
    auto _iter = _this->_global_ignore_variables.find(_key_data);
    if(_iter == _this->_global_ignore_variables.end()){
      _variable_data* _vdata = new _variable_data();
      _vdata->var_name = _key_data->to_string();
      _vdata->var_data = to_variant(state, value_stack_idx);
      _vdata->lua_type = lua_type(state, value_stack_idx);

      _this->_vdata_list.insert(_this->_vdata_list.end(), _vdata);
      _this->_vdata_map[_vdata->var_name] = _vdata;
    }

    cpplua_delete_variant(_key_data);
  }, this);

  lua_pop(_state, 1);

  return true;
}

void variable_watcher::update_global_table_ignore(){
  _global_ignore_variables.clear();

  lua_pushglobaltable(_state);

  // find all key
  iterate_table(_state, -1, [](lua_State* state, int key_stack_idx, int value_stack_idx, int iter_idx, void* cb_data){
    variable_watcher* _this = (variable_watcher*)cb_data;
    
    variant* _key_data = to_variant(state, key_stack_idx);
    _this->_global_ignore_variables.insert(_key_data);

    cpplua_delete_variant(_key_data);
  }, this);

  lua_pop(_state, 1);
}


int variable_watcher::get_variable_count() const{
  return _vdata_list.size();
}


const char* variable_watcher::get_variable_name(int idx) const{
  if(idx < 0 || idx >= _vdata_list.size())
    return NULL;

  return _vdata_list[idx]->var_name.c_str();
}


I_variant* variable_watcher::get_variable(int idx) const{
  if(idx < 0 || idx >= _vdata_list.size())
    return NULL;

  return _vdata_list[idx]->var_data;
}

I_variant* variable_watcher::get_variable(const char* name) const{
  auto _iter = _vdata_map.find(name);
  if(_iter == _vdata_map.end())
    return NULL;

  return _iter->second->var_data;
}


int variable_watcher::get_variable_type(int idx) const{
  if(idx < 0 || idx >= _vdata_list.size())
    return LUA_TNIL;

  return _vdata_list[idx]->lua_type;
}

int variable_watcher::get_variable_type(const char* name) const{
  auto _iter = _vdata_map.find(name);
  if(_iter == _vdata_map.end())
    return LUA_TNIL;

  return _iter->second->lua_type;
}


void variable_watcher::set_logger(I_logger* logger){
  _logger = logger;
}



// MARK: DLL functions

DLLEXPORT I_variable_watcher* CPPLUA_CREATE_VARIABLE_WATCHER(void* interface_state){
  return new variable_watcher((lua_State*)interface_state);
}

DLLEXPORT void CPPLUA_DELETE_VARIABLE_WATCHER(I_variable_watcher* watcher){
  delete watcher;
}