#include "luaapi_core.h"
#include "luainternal_storage.h"
#include "luamemory_util.h"
#include "luaruntime_handler.h"
#include "luastate_util.h"
#include "luathread_control.h"
#include "luathread_util.h"
#include "luautility.h"
#include "luavariant.h"
#include "luavariant_arr.h"
#include "luavariant_util.h"
#include "std_logger.h"
#include "string_util.h"

#define LUD_RUNTIME_HANDLER_VAR_NAME "__clua_runtime_handler"
#define LUD_RUNTIME_HANDLER_LUA_CHUNK_FUNC "__clua_runtime_handler_main_func"

// Maximum waiting to stop.
#define RUNTIME_HANDLER_MAXIMUM_WAIT_THREAD 1000


using namespace dynamic_library::util;
using namespace lua;
using namespace lua::api;
using namespace lua::debug;
using namespace lua::internal;
using namespace lua::memory;
using namespace lua::utility;
using namespace ::memory;


#ifdef LUA_CODE_EXISTS

static const I_dynamic_management* __dm = get_memory_manager();


// MARK: lua::runtime_handler
runtime_handler::runtime_handler(bool load_library){
  _initiate_constructor();
  if(load_library)
    load_std_libs();

  _initiate_class();
}

runtime_handler::runtime_handler(const std::string& lua_path, bool load_library){
  _initiate_constructor();
  int _err_code;
  if((_err_code = load_file(lua_path.c_str())) != LUA_OK && _logger){
    string_var _vstr = format_str_mem(__dm, "[runtime_handler] Cannot load file '%s'. Error Code: %d\n", lua_path.c_str(), _err_code);
    _logger->print_error(_vstr.get_string());
    _set_error_obj(&_vstr);
  }

  if(load_library)
    load_std_libs();

  _initiate_class();
}


// NOTE: when stopping a lua execution, try to step for a line, and then quit
// This approach might reduce the likeliness of a bug
runtime_handler::~runtime_handler(){
  if(!_state)
    return;

  if(currently_running())
    stop_running();

  __dm->delete_class_dbg(_thread_control, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  __dm->delete_class_dbg(_library_loader, DYNAMIC_MANAGEMENT_DEBUG_DATA);

  __dm->delete_class_dbg(_main_func, DYNAMIC_MANAGEMENT_DEBUG_DATA);

  _set_bind_obj(NULL, _state);
  reset_last_error();

  lua_close(_state);

#if (_WIN64) || (_WIN32)
  DeleteCriticalSection(&_obj_mutex);
#endif
}


void runtime_handler::_initiate_constructor(){
  _logger = get_std_logger();
  _state = newstate();

  _main_func = __dm->new_class_dbg<function_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
}

void runtime_handler::_initiate_class(){
#if (_WIN64) || (_WIN32)
  InitializeCriticalSection(&_obj_mutex);
#endif

  _thread_control = __dm->new_class_dbg<thread_control>(DYNAMIC_MANAGEMENT_DEBUG_DATA, this);
  _library_loader = __dm->new_class_dbg<library_loader>(DYNAMIC_MANAGEMENT_DEBUG_DATA, _state);

  _set_bind_obj(this, _state);
}


void runtime_handler::_lock_object(){
#if (_WIN64) || (_WIN64)
  EnterCriticalSection(&_obj_mutex);
#endif
}

void runtime_handler::_unlock_object(){
#if (_WIN64) || (_WIN64)
  LeaveCriticalSection(&_obj_mutex);
#endif
}


void runtime_handler::_read_error_obj(){
  _lock_object();
  if(_last_err_obj)
    cpplua_delete_variant(_last_err_obj);

  _last_err_obj = to_variant(_state, -1);
  lua_pop(_state, 1);
  _unlock_object();
}

void runtime_handler::_set_error_obj(const I_variant* err_data){
  _lock_object();
  if(_last_err_obj)
    cpplua_delete_variant(_last_err_obj);
  
  if(err_data)
    _last_err_obj = cpplua_create_var_copy(err_data);
  else
    _last_err_obj = NULL;
  _unlock_object();
}


void runtime_handler::_set_bind_obj(runtime_handler* obj, lua_State* state){
  require_internal_storage(state); // s+1

  lua_pushstring(state, LUD_RUNTIME_HANDLER_VAR_NAME); // s+1
  lua_pushlightuserdata(state, obj); // s+1
  lua_settable(state, -3); // s-2

  // pop internal storage
  lua_pop(state, 1); // s-1
}


runtime_handler* runtime_handler::get_attached_object(lua_State* state){
  runtime_handler* _result = NULL;
  require_internal_storage(state); // s+1

  lua_pushstring(state, LUD_RUNTIME_HANDLER_VAR_NAME); // s+1
  lua_gettable(state, -2); // s-1+1
  if(lua_type(state, -1) == LUA_TLIGHTUSERDATA)
    _result = (runtime_handler*)lua_touserdata(state, -1);

  // pop internal storage and gettable result
  lua_pop(state, 2);
  return _result;
}


lua_State* runtime_handler::get_lua_state() const{
  return _state;
}

void* runtime_handler::get_lua_state_interface() const{
  return _state;
}

core runtime_handler::get_lua_core_copy() const{
  return as_lua_api_core(_state);
}


thread_control* runtime_handler::get_thread_control() const{
  return _thread_control;
}

library_loader* runtime_handler::get_library_loader() const{
  return _library_loader;
}


I_thread_control* runtime_handler::get_thread_control_interface() const{
  return _thread_control;
}

I_library_loader* runtime_handler::get_library_loader_interface() const{
  return _library_loader;
}


bool runtime_handler::run_code(){
  if(!_state)
    return false;

  if(currently_running())
    return false;

  _current_tid = _thread_control->run_execution([](void* data){
    thread_handle* _this_thread = get_thread_handle();

    runtime_handler* _this = (runtime_handler*)data;
    core _lc = _this->get_lua_core_copy();
    const I_function_var* _main_func = _this->get_main_function();
    
    vararr _args, _result;
    int _errcode = _main_func->run_function(&_lc, &_args, &_result);
    if(_this_thread->is_stop_signal())
      return;
    
    if(_errcode != LUA_OK && _result.get_var_count() > 0){
      _this->_set_error_obj(_result.get_var(0));
      if(_this->_logger)
        _this->_logger->print_error(format_str_mem(__dm, "[runtime_handler] Error when running Lua file: %s\n", _this->_last_err_obj->to_string().c_str()).c_str());
    }
  }, this);
  
  return true;
}

bool runtime_handler::currently_running() const{
  if(!_state || _current_tid == ERROR_INVALID_THREAD_ID)
    return false;

  I_thread_handle_reference* _ref = _thread_control->get_thread_handle(_current_tid);
  if(!_ref)
    return false;

  bool _result = !_ref->get_interface()->is_stopped();
  _thread_control->free_thread_handle(_ref);
  return _result;
}

bool runtime_handler::stop_running(){
  if(!_state)
    return false;

  I_thread_handle_reference* _ref = _thread_control->get_thread_handle(_current_tid);
  if(!_ref)
    return false;

  _ref->get_interface()->stop_running();
  return true;
}

unsigned long runtime_handler::get_main_thread_id() const{
  if(!_state || !currently_running())
    return ERROR_INVALID_THREAD_ID;

  return _current_tid;
}


const I_function_var* runtime_handler::get_main_function() const{
  return _main_func;
}


int runtime_handler::load_file(const char* lua_path){
  if(!_state || currently_running())
    return -1;

  int _err_code = luaL_loadfile(_state, lua_path); // main function s+1
  if(_err_code != LUA_OK){
    _read_error_obj();
    return _err_code;
  }

  core _lc = as_lua_api_core(_state);
  // copy to variant
  _main_func->from_state(&_lc, -1);
  
  // set as global variable
  require_internal_storage(_state); // s+1

  lua_pushstring(_state, LUD_RUNTIME_HANDLER_LUA_CHUNK_FUNC); // s+1
  lua_pushvalue(_state, -3); // copy main function s+1
  lua_settable(_state, -3); // s-2

  lua_pop(_state, 2); // pop main function and internal storage
  return LUA_OK;
}

void runtime_handler::load_std_libs(){
  if(!_state)
    return;

  luaL_openlibs(_state);
}


const lua::I_variant* runtime_handler::get_last_error_object() const{
  return _last_err_obj;
}

void runtime_handler::reset_last_error(){
  _set_error_obj();
}


void runtime_handler::set_logger(I_logger* logger){
  _logger = logger;

  _thread_control->set_logger(logger);
  _library_loader->set_logger(logger);
}



// MARK: DLL functions

DLLEXPORT lua::I_runtime_handler* CPPLUA_CREATE_RUNTIME_HANDLER(const char* lua_path, bool load_library){
  if(lua_path)
    return __dm->new_class_dbg<runtime_handler>(DYNAMIC_MANAGEMENT_DEBUG_DATA, lua_path, load_library);
  else
    return __dm->new_class_dbg<runtime_handler>(DYNAMIC_MANAGEMENT_DEBUG_DATA, load_library);
}

DLLEXPORT void CPPLUA_DELETE_RUNTIME_HANDLER(I_runtime_handler* handler){
  __dm->delete_class_dbg(handler, DYNAMIC_MANAGEMENT_DEBUG_DATA);
}

#endif // LUA_CODE_EXISTS