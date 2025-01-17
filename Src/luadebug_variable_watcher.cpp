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

#if (_WIN64) | (_WIN32)
#include "Windows.h"
#endif

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

#if (_WIN64) | (_WIN32)
static CRITICAL_SECTION* _code_mutex = NULL;
#endif

static void _code_initiate();
static void _code_deinitiate();
static destructor_helper _dh(_code_deinitiate);

static void _lock_code();
static void _unlock_code();


static void _code_initiate(){
  if(!_code_mutex){
    _code_mutex = (CRITICAL_SECTION*)__dm->malloc(sizeof(CRITICAL_SECTION), DYNAMIC_MANAGEMENT_DEBUG_DATA);
    InitializeCriticalSection(_code_mutex);
  }

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
    DeleteCriticalSection(_code_mutex);
    __dm->free(_code_mutex, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _code_mutex = NULL;
  }

  if(_internal_variables_list){
    __dm->delete_class_dbg(_internal_variables_list, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _internal_variables_list = NULL;
  }
}


void _lock_code(){
#if (_WIN64) | (_WIN32)
  EnterCriticalSection(_code_mutex);
#endif
}

void _unlock_code(){
#if (_WIN64) | (_WIN32)
  LeaveCriticalSection(_code_mutex);
#endif
}


// MARK: lua::debug::variable_watcher
variable_watcher::variable_watcher(lua_State* state){
  _code_initiate();

  _logger = get_std_logger();
  _state = state;

#if (_WIN64) || (_WIN32)
  _object_mutex_ptr = &_object_mutex;
  InitializeCriticalSection(_object_mutex_ptr);
#endif
}

variable_watcher::~variable_watcher(){
  _clear_variable_data();

#if (_WIN64) || (_WIN32)
  DeleteCriticalSection(_object_mutex_ptr);
#endif
}


void variable_watcher::_lock_object() const{
#if (_WIN64) || (_WIN32)
  EnterCriticalSection(_object_mutex_ptr);
#endif
}

void variable_watcher::_unlock_object() const{
#if (_WIN64) || (_WIN32)
  LeaveCriticalSection(_object_mutex_ptr);
#endif
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


void variable_watcher::_clear_variable_data(){
  for(_variable_data* _data: _vdata_list){
    cpplua_delete_variant(_data->var_data);
    __dm->delete_class_dbg(_data, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  }

  _vdata_list.clear();
  _vdata_map.clear();
}


bool variable_watcher::fetch_current_function_variables(){
  bool _result = true;
  lua_Debug* _debug_data = (lua_Debug*)__dm->malloc(sizeof(lua_Debug), DYNAMIC_MANAGEMENT_DEBUG_DATA);

  _lock_state();
{ // enclosure for using gotos
  if(!lua_getstack(_state, 0, _debug_data)){
    if(_logger)
      _logger->print_error("[variable_watcher] Cannot get current function stack info.\n");

    _result = false;
    goto cleanup_label;
  }

  _clear_variable_data();

  int i = 0;
  while(true){
    const char* _var_name_str = lua_getlocal(_state, _debug_data, i+1);
    if(!_var_name_str)
      break;

    _variable_data* _vdata = __dm->new_class_dbg<_variable_data>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _vdata->var_name = _var_name_str;
    _vdata->var_data = to_variant(_state, -1);
    _vdata->lua_type = lua_type(_state, -1);

    lua_pop(_state, 1);

    _vdata_list.insert(_vdata_list.end(), _vdata);
    _vdata_map[_var_name_str] = _vdata;

    i++;
  }
} // enclosure closing

  cleanup_label:{
    __dm->free(_debug_data, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  }

  _unlock_state();
  return _result;
}


bool variable_watcher::fetch_global_table_data(){
  _lock_code();
  _lock_state();
  _clear_variable_data();

  lua_pushglobaltable(_state);

  iterate_table(_state, -1, [](const core* lc, int key_stack_idx, int value_stack_idx, int iter_idx, void* cb_data){
    variable_watcher* _this = (variable_watcher*)cb_data;
    lua_State* _state = (lua_State*)lc->istate;
    variant* _key_data;

{ // enclosure for using goto
    _key_data = to_variant(_state, key_stack_idx);

    if(_this->_ignore_internal_variables){
      auto _iter_internal = _internal_variables_list->find(_key_data);
      if(_iter_internal != _internal_variables_list->end())
        goto skip_to_clean;
    }

    auto _iter_gvar = _this->_global_ignore_variables.find(_key_data);
    if(_iter_gvar != _this->_global_ignore_variables.end())
      goto skip_to_clean;

    _variable_data* _vdata = __dm->new_class_dbg<_variable_data>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _vdata->var_name = _key_data->to_string();
    _vdata->var_data = to_variant(_state, value_stack_idx);
    _vdata->lua_type = lua_type(_state, value_stack_idx);

    _this->_vdata_list.insert(_this->_vdata_list.end(), _vdata);
    _this->_vdata_map[_vdata->var_name] = _vdata;
} // enclosure closing

    skip_to_clean:{}

    cpplua_delete_variant(_key_data);
  }, this);

  lua_pop(_state, 1);

  _unlock_state();
  _unlock_code();
  return true;
}

void variable_watcher::update_global_table_ignore(){
  _lock_state();
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

void variable_watcher::clear_global_table_ignore(){
  _lock_object();
  _global_ignore_variables.clear();

  _unlock_object();
}


void variable_watcher::ignore_internal_variables(bool flag){
  _ignore_internal_variables = flag;
}

bool variable_watcher::is_ignore_internal_variables() const{
  return _ignore_internal_variables;
}


int variable_watcher::get_variable_count() const{
  int _result;
  _lock_object();
  _result = _vdata_list.size();
  _unlock_object();
  return _result;
}


const char* variable_watcher::get_variable_name(int idx) const{
  const char* _result = NULL;
  _lock_object();
  if(idx < 0 || idx >= _vdata_list.size())
    goto skip_to_return;

  _result = _vdata_list[idx]->var_name.c_str();

  skip_to_return:{}
  _unlock_object();
  return _result;
}


I_variant* variable_watcher::get_variable(int idx) const{
  I_variant* _result = NULL;
  _lock_object();
  if(idx < 0 || idx >= _vdata_list.size())
    goto skip_to_return;

  _result = _vdata_list[idx]->var_data;
  
  skip_to_return:{}
  _unlock_object();
  return _result;
}

I_variant* variable_watcher::get_variable(const char* name) const{
  I_variant* _result = NULL;
  _lock_object();
  auto _iter = _vdata_map.find(name);
  if(_iter == _vdata_map.end())
    goto skip_to_return;

  _result = _iter->second->var_data;

  skip_to_return:{}
  _unlock_object();
  return _result;
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

int variable_watcher::get_variable_type(const char* name) const{
  int _result = LUA_TNIL;
  _lock_object();
  auto _iter = _vdata_map.find(name);
  if(_iter == _vdata_map.end())
    goto skip_to_return;

  _result = _iter->second->lua_type;
  
  skip_to_return:{}
  _unlock_object();
  return _result;
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