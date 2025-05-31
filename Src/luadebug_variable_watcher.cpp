#include "error_definition.h"
#include "dllutil.h"
#include "library_linking.h"
#include "luaapi_variant_util.h"
#include "luadebug_variable_watcher.h"
#include "luainternal_storage.h"
#include "luamemory_util.h"
#include "luastate_util.h"
#include "luatable_util.h"
#include "luathread_util.h"
#include "luautility.h"
#include "luavariant_util.h"
#include "std_logger.h"
#include "string_util.h"

#include "mutex"


using namespace dynamic_library::util;
using namespace lua;
using namespace lua::api;
using namespace lua::debug;
using namespace lua::internal;
using namespace lua::memory;
using namespace lua::thread;
using namespace lua::utility;
using namespace ::memory;


#ifdef LUA_CODE_EXISTS

static const I_dynamic_management* __dm = get_memory_manager();
static std::set<comparison_variant>* _internal_variables_list = NULL;

std::recursive_mutex* _code_mutex = NULL;


static void _code_initiate();
static void _code_deinitiate();
static destructor_helper _dh(_code_deinitiate);

static void _lock_code();
static void _unlock_code();


static void _code_initiate(){
  if(!_code_mutex)
    _code_mutex = new std::recursive_mutex();

  if(!_internal_variables_list){
    _internal_variables_list = __dm->new_class_dbg<std::set<comparison_variant>>(DYNAMIC_MANAGEMENT_DEBUG_DATA);

    lua_State* _tmpstate = NULL;
    variant* _gvar = NULL;

{ // enclosure for using goto
    _tmpstate = newstate();
    if(!_tmpstate)
      goto skip_to_clean;

    luaL_loadstring(_tmpstate, "");
    luaL_openlibs(_tmpstate);

    core _lc = as_lua_api_core(_tmpstate);
    lua_pushglobaltable(_tmpstate);

    variant* _gvar = to_variant(&_lc, -1);
    if(_gvar->get_type() != I_table_var::get_static_lua_type())
      goto skip_to_clean;

    table_var* _tgvar = dynamic_cast<table_var*>(_gvar);
    const I_variant** _gkey_list = _tgvar->get_keys();
    for(int i = 0; _gkey_list[i]; i++){
      const I_variant* _key = _gkey_list[i];
      _internal_variables_list->insert(_key);
    }
} // enclosure closing

    skip_to_clean:{}
    if(_gvar)
      cpplua_delete_variant(_gvar);

    if(_tmpstate)
      lua_close(_tmpstate);
  }
}

static void _code_deinitiate(){
  if(_code_mutex){
    delete _code_mutex;
    _code_mutex = NULL;
  }

  if(_internal_variables_list){
    __dm->delete_class_dbg(_internal_variables_list, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _internal_variables_list = NULL;
  }
}


void _lock_code(){
  _code_mutex->lock();
}

void _unlock_code(){
  _code_mutex->unlock();
}


// MARK: lua::debug::variable_watcher
variable_watcher::variable_watcher(lua_State* state){
  _code_initiate();

  _logger = get_std_logger();
  _state = state;

  _object_mutex_ptr = &_object_mutex;
}

variable_watcher::~variable_watcher(){
  _clear_variable_data();
}


void variable_watcher::_lock_object() const{
  _object_mutex_ptr->lock();
}

void variable_watcher::_unlock_object() const{
  _object_mutex_ptr->unlock();
}


void variable_watcher::_lock_state() const{
  _lock_object();
  lock_state(_state);
  thread_dependent_enable(_state, false);
}

void variable_watcher::_unlock_state() const{
  thread_dependent_enable(_state, true);
  unlock_state(_state);
  _unlock_object();
}


I_variant* variable_watcher::_get_variable_value(int idx) const{
  I_variant* _result = NULL;
  _lock_object();
  if(idx < 0 || idx >= _vdata_list.size())
    goto skip_to_return;

  _result = _vdata_list[idx]->var_data;
  
  skip_to_return:{}
  _unlock_object();
  return _result;
}

I_variant* variable_watcher::_get_variable_value(const I_variant* key) const{
  I_variant* _result = NULL;
  _lock_object();
  auto _iter = _vdata_map.find(key);
  if(_iter == _vdata_map.end())
    goto skip_to_return;

  _result = _iter->second->var_data;

  skip_to_return:{}
  _unlock_object();
  return _result;
}


void variable_watcher::_set_last_error(const I_variant* err_obj, int err_code){
  _clear_last_error();
  
  _current_error = __dm->new_class_dbg<error_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, err_obj, err_code);
}

void variable_watcher::_clear_last_error(){
  if(!_current_error)
    return;

  __dm->delete_class_dbg(_current_error, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  _current_error = NULL;
}


void variable_watcher::_clear_variable_data(){
  for(_variable_data* _data: _vdata_list){
    cpplua_delete_variant(_data->var_key);
    cpplua_delete_variant(_data->var_data);
    __dm->delete_class_dbg(_data, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  }

  _vdata_list.clear();
  _vdata_map.clear();
}

void variable_watcher::_clear_local_variable_data(){
  for(auto _pair: _vlocaldata_map)
    __dm->delete_class_dbg(_pair.second, DYNAMIC_MANAGEMENT_DEBUG_DATA);

  _vlocaldata_map.clear();
  _current_local_idx = -1;
}

void variable_watcher::_clear_object(){
  _clear_variable_data();
  _clear_last_error();
}


void variable_watcher::lock_object() const{
  _lock_object();
}

void variable_watcher::unlock_object() const{
  _unlock_object();
}


void variable_watcher::update_global_ignore(){
  _unlock_state();
  _global_ignore_variables.clear();

  lua_pushglobaltable(_state);

  // find all key
  iterate_table(_state, -1, [](const core* lc, int key_stack_idx, int value_stack_idx, int iter_idx, void* cb_data){
    variable_watcher* _this = (variable_watcher*)cb_data;
    lua_State* _state = (lua_State*)lc->istate;
    
    variant* _key_data = to_variant(_state, key_stack_idx);
    _this->_global_ignore_variables.insert(_key_data);

    cpplua_delete_variant(_key_data);
  }, this);

  lua_pop(_state, 1);
  _unlock_state();
}

void variable_watcher::clear_global_ignore(){
  _lock_object();
  _global_ignore_variables.clear();

  _unlock_object();
}


bool variable_watcher::update_global_variables(){
  _lock_state();
  _clear_variable_data();

  lua_pushglobaltable(_state);

  iterate_table(_state, -1, [](const core* lc, int key_stack_idx, int value_stack_idx, int iter_idx, void* cb_data){
    variable_watcher* _this = (variable_watcher*)cb_data;
    lua_State* _state = (lua_State*)lc->istate;

    variant* _key_data = to_variant(_state, key_stack_idx);
    if(_this->_ignore_internal_variables){
      auto _internal_iter = _internal_variables_list->find(_key_data);
      if(_internal_iter != _internal_variables_list->end())
        return;
    }

    auto _global_iter = _this->_global_ignore_variables.find(_key_data);
    if(_global_iter != _this->_global_ignore_variables.end())
      return;

    _variable_data* _vdata = __dm->new_class_dbg<_variable_data>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _vdata->var_key = _key_data;
    _vdata->var_data = to_variant(_state, value_stack_idx);
    _vdata->lua_type = lua_type(_state, value_stack_idx);

    _this->_vdata_list.insert(_this->_vdata_list.end(), _vdata);
    _this->_vdata_map[_key_data] = _vdata;
  }, this);

  lua_pop(_state, 1);

  _unlock_state();
  return true;
}

bool variable_watcher::update_local_variables(int stack_idx){
  bool _result = true;
  lua_Debug* _debug_data = (lua_Debug*)__dm->malloc(sizeof(lua_Debug), DYNAMIC_MANAGEMENT_DEBUG_DATA);

  _lock_state();

{ // enclosure for using gotos
  if(!lua_getstack(_state, stack_idx, _debug_data)){
    if(_logger)
      _logger->print_error("[variable_watcher] Cannot get current function stack info.\n");

    _result = false;
    goto cleanup_label;
  }

  _clear_variable_data();
  _clear_local_variable_data();

  _current_local_idx = stack_idx;

  int i = 1;
  while(true){
    const char* _var_name_str = lua_getlocal(_state, _debug_data, i);
    if(!_var_name_str)
      break;

    string_var _key_str = _var_name_str;
    variant* _key_var = cpplua_create_var_copy(&_key_str);

    _variable_data* _vdata = __dm->new_class_dbg<_variable_data>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _vdata->var_key = _key_var;
    _vdata->var_data = to_variant(_state, -1);
    _vdata->lua_type = lua_type(_state, -1);

    _local_variable_data* _vlocal_data = __dm->new_class_dbg<_local_variable_data>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _vlocal_data->local_idx = i;

    lua_pop(_state, 1);

    _vdata_list.insert(_vdata_list.end(), _vdata);
    _vdata_map[_key_var] = _vdata;
    _vlocaldata_map[_key_var] = _vlocal_data;

    i++;
  }
} // enclosure closing

  cleanup_label:{
    __dm->free(_debug_data, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  }

  _unlock_state();
  return _result;
}


void variable_watcher::ignore_internal_variables(bool flag){
  _ignore_internal_variables = flag;
}

bool variable_watcher::is_ignore_internal_variables() const{
  return _ignore_internal_variables;
}


bool variable_watcher::is_internal_variables(const I_variant* key) const{
  auto _iter = _internal_variables_list->find(key);
  return _iter != _internal_variables_list->end();
}


int variable_watcher::get_variable_count() const{
  int _result;
  _lock_object();
  _result = _vdata_list.size();
  _unlock_object();
  return _result;
}

const I_variant* variable_watcher::get_variable_key(int idx) const{
  const I_variant* _result = NULL;
  _lock_object();
  if(idx < 0 || idx >= _vdata_list.size())
    goto skip_to_return;

  _result = _vdata_list[idx]->var_key;

  skip_to_return:{}
  _unlock_object();
  return _result;
}

I_variant* variable_watcher::get_variable_value_mutable(int idx){
  return _get_variable_value(idx);
}

I_variant* variable_watcher::get_variable_value_mutable(const I_variant* key){
  return _get_variable_value(key);
}

const I_variant* variable_watcher::get_variable_value(int idx) const{
  return _get_variable_value(idx);
}

const I_variant* variable_watcher::get_variable_value(const I_variant* key) const{
  return _get_variable_value(key);
}

int variable_watcher::get_variable_type(int idx) const{
  int _result = LUA_TNIL;
  _lock_object();
  if(idx < 0 || idx >= _vdata_list.size())
    goto skip_to_return;

  _result = _vdata_list[idx]->lua_type;
  
  skip_to_return:{}
  _unlock_object();
  return _result;
}

int variable_watcher::get_variable_type(const I_variant* key) const{
  int _result = LUA_TNIL;
  _lock_object();
  auto _iter = _vdata_map.find(key);
  if(_iter == _vdata_map.end())
    goto skip_to_return;

  _result = _iter->second->lua_type;
  
  skip_to_return:{}
  _unlock_object();
  return _result;
}


bool variable_watcher::set_global_variable(const I_variant* key, const I_variant* value){
  _lock_state();
  core _lc = as_lua_api_core(_state);
  
  lua_pushglobaltable(_state);

  key->push_to_stack(&_lc);
  value->push_to_stack(&_lc);
  lua_settable(_state, -3);

  // pops global table
  lua_pop(_state, 1);

  _unlock_state();

  return true;
}

bool variable_watcher::set_local_variable(const I_variant* key, const I_variant* value){
  bool _result = true;
  lua_Debug* _debug_data = (lua_Debug*)__dm->malloc(sizeof(lua_Debug), DYNAMIC_MANAGEMENT_DEBUG_DATA);

  _lock_state();
  core _lc = as_lua_api_core(_state);

{ // enclosure for using gotos
  // case: not yet updated
  if(_current_local_idx < 0){
    string_var _err_str = "[variable_watcher] Cannot fetch stack data; not in local variable mode?";
    _set_last_error(&_err_str, ERROR_INVALID_FUNCTION);

    _result = false;
    goto cleanup_label;
  }

  if(!lua_getstack(_state, _current_local_idx, _debug_data)){
    string_var _err_str = "[variable_watcher] Cannot get current function stack info.\n";
    _set_last_error(&_err_str, ERROR_INVALID_PARAMETER);

    _result = false;
    goto cleanup_label;
  }

  auto _iter = _vlocaldata_map.find(key);
  if(_iter == _vlocaldata_map.end()){
    string_store _var_str; key->to_string(&_var_str);
    string_var _err_str = format_str("[variable_watcher] Cannot find '%s' local variable in stack idx %d.", _var_str.data.c_str(), _current_local_idx);
    _set_last_error(&_err_str, ERROR_NOT_FOUND);

    _result = false;
    goto cleanup_label;
  }

  value->push_to_stack(&_lc);
  const char* _var_name = lua_setlocal(_state, _debug_data, _iter->second->local_idx);
  if(!_var_name){
    string_store _var_str; key->to_string(&_var_str);
    string_var _err_str = format_str("[variable_watcher] Cannot set '%s' variable in stack idx %d.", _var_str.data.c_str(), _current_local_idx);
    _set_last_error(&_err_str, ERROR_INVALID_PARAMETER);

    _result = false;
    goto cleanup_label;
  }

} // enclosure closing

  cleanup_label:{
    __dm->free(_debug_data, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  }

  _unlock_state();
  return _result;
}


int variable_watcher::get_current_local_stack_idx() const{
  return _current_local_idx;
}


const I_error_var* variable_watcher::get_last_error() const{
  return _current_error;
}


void variable_watcher::set_logger(I_logger* logger){
  _logger = logger;
}



// MARK: DLL functions

DLLEXPORT I_variable_watcher* CPPLUA_CREATE_VARIABLE_WATCHER(void* interface_state){
  return __dm->new_class_dbg<variable_watcher>(DYNAMIC_MANAGEMENT_DEBUG_DATA, (lua_State*)interface_state);
}

DLLEXPORT void CPPLUA_DELETE_VARIABLE_WATCHER(I_variable_watcher* watcher){
  __dm->delete_class_dbg(watcher, DYNAMIC_MANAGEMENT_DEBUG_DATA);
}

#endif // LUA_CODE_EXISTS