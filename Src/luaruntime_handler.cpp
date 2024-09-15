#include "lua_variant.h"
#include "luaruntime_handler.h"
#include "stdlogger.h"

#define LUD_RUNTIME_HANDLER_VAR_NAME "__clua_runtime_handler"
#define LUD_RUNTIME_HANDLER_LUA_CHUNK_FUNC "__clua_runtime_handler_main_func"


using namespace lua;
using namespace lua::debug;



// MARK: lua::runtime_handler

runtime_handler::runtime_handler(lua_State* state){
  _create_own_lua_state = false;

  _logger = get_stdlogger();
  _state = NULL;

#if (_WIN64) || (_WIN32)
  _thread_handle = NULL;
#endif

  runtime_handler* _current_handler = get_attached_object(state);
  if(_current_handler){
    _logger->print_error("[runtime_handler] Cannot intialize. Reason: lua_State already has runtime_handler.\n");
    return;
  }

  _state = state;
  _initiate_class();
}


runtime_handler::runtime_handler(bool load_library){
  _initiate_constructor();
  if(load_library)
    load_std_libs();

  _initiate_class();
}

runtime_handler::runtime_handler(const std::string& lua_path, bool immediate_run, bool load_library){
  _initiate_constructor();
  int _err_code;
  if((_err_code = load_file(lua_path.c_str())) != LUA_OK && _logger){
    _logger->print_error(format_str("[runtime_handler] Cannot load file '%s'. Error Code: %d\n", lua_path.c_str(), _err_code));
  }

  if(load_library)
    load_std_libs();

  if(immediate_run && _err_code == LUA_OK){
    if((_err_code = run_current_file()) != LUA_OK && _logger){
      _logger->print_error(format_str("[runtime_handler] Something went wrong when running lua file '%s'. Error Code: %d\n", lua_path.c_str(), _err_code));
    }
  }

  _initiate_class();
}


// NOTE: when stopping a lua execution, try to step for a line, and then quit
// This approach might reduce the likeliness of a bug
runtime_handler::~runtime_handler(){
  if(!_state)
    return;

  if(_execution_flow->currently_pausing())
    _execution_flow->resume_execution();
    
  stop_execution();

  _hook_handler->remove_hook(this);
  _set_bind_obj(NULL, _state);

  delete _execution_flow;
  delete _function_database;
  delete _hook_handler;
  delete _library_loader;

  if(_create_own_lua_state)
    lua_close(_state);

  if(_last_err_obj)
    delete _last_err_obj;
}


#if (_WIN64) || (_WIN32)
void runtime_handler::_deinit_thread(){
  for(auto _event: _event_finished)
    SetEvent(_event);

  delete _thread_data;
  _thread_data = NULL;
}

DWORD __stdcall runtime_handler::_thread_entry_point(LPVOID data){
  _t_entry_point_data* _ep_data = (_t_entry_point_data*)data;

  execution_context _context = _ep_data->cb;
  void* _context_data = _ep_data->cbdata;

  _context(_context_data);

  _ep_data->_this->_deinit_thread();
  return 0;
}
#endif


void runtime_handler::_initiate_constructor(){
  _create_own_lua_state = true;
  
  _logger = get_stdlogger();
  _state = luaL_newstate();

#if (_WIN64) || (_WIN32)
  _thread_handle = NULL;
#endif
}

void runtime_handler::_initiate_class(){
  _set_bind_obj(this, _state);

  _hook_handler = new hook_handler(_state);
  _hook_handler->set_hook(LUA_MASKLINE, _hookcb_static, this);

  _execution_flow = new execution_flow(_state);

  _function_database = new func_db(_state);

  _library_loader = new lib_loader(_state);
}


void runtime_handler::_read_error_obj(){
  if(_last_err_obj)
    cpplua_delete_variant(_last_err_obj);

  _last_err_obj = to_variant(_state, -1);
  lua_pop(_state, 1);
}


void runtime_handler::_hookcb(){
  const lua_Debug* _debug_data = _hook_handler->get_current_debug_value();
  if(_debug_data->event != LUA_HOOKLINE)
    return;

#if (_WIN64) || (_WIN32)
  if(!_stop_thread || GetThreadId(_thread_handle) != GetCurrentThreadId())
    return;

  _deinit_thread();
  ExitThread(0);
#endif
}


void runtime_handler::_set_bind_obj(runtime_handler* obj, lua_State* state){
  lightuser_var _lud_var = obj;
  _lud_var.setglobal(state, LUD_RUNTIME_HANDLER_VAR_NAME);
}


void runtime_handler::_hookcb_static(lua_State* state, void* cbdata){
  ((runtime_handler*)cbdata)->_hookcb();
}


runtime_handler* runtime_handler::get_attached_object(lua_State* state){
  variant* _lud_var = to_variant_fglobal(state, LUD_RUNTIME_HANDLER_VAR_NAME);
  runtime_handler* _result = 
    (runtime_handler*)(_lud_var->get_type() == lightuser_var::get_static_lua_type()?((lightuser_var*)_lud_var)->get_data(): NULL);

  delete _lud_var;
  return _result;
}


lua_State* runtime_handler::get_lua_state(){
  return _state;
}

void* runtime_handler::get_lua_state_interface(){
  return _state;
}


func_db* runtime_handler::get_function_database(){
  return _function_database;
}

hook_handler* runtime_handler::get_hook_handler(){
  return _hook_handler;
}

execution_flow* runtime_handler::get_execution_flow(){
  return _execution_flow;
}

lib_loader* runtime_handler::get_library_loader(){
  return _library_loader;
}


I_func_db* runtime_handler::get_function_database_interface(){
  return _function_database;
}

I_hook_handler* runtime_handler::get_hook_handler_interface(){
  return _hook_handler;
}

I_execution_flow* runtime_handler::get_execution_flow_interface(){
  return _execution_flow;
}

I_lib_loader* runtime_handler::get_library_loader_interface(){
  return _library_loader;
}


#if (_WIN64) || (_WIN32)
// ret_val keep empty if returns void
#define PROHIBIT_SAME_THREAD_EXECUTION_CHECK(ret_val) \
  if(get_running_thread_id() == GetCurrentThreadId()){ \
    if(_logger) \
      _logger->print_error("[runtime_handler] Calling function " __FUNCTION__ " using same running thread is prohibited.\n"); \
    return ret_val; \
  }
#endif

#define VOID_RET 

void runtime_handler::stop_execution(){
  if(!_thread_handle)
    return;

  PROHIBIT_SAME_THREAD_EXECUTION_CHECK(VOID_RET)

#if (_WIN64) || (_WIN32)
  if(is_currently_executing()){
    _execution_flow->resume_execution();

    _stop_thread = true;
    WaitForSingleObject(_thread_handle, INFINITE);
  }
  
  CloseHandle(_thread_handle);
  _thread_handle = NULL;
#endif
}

bool runtime_handler::run_execution(execution_context cb, void* cbdata){
  if(is_currently_executing())
    return false;

  PROHIBIT_SAME_THREAD_EXECUTION_CHECK(false)

  // deinit thread
  stop_execution();

#if (_WIN64) || (_WIN32)
  _t_entry_point_data* _ep_data = new _t_entry_point_data();
  _ep_data->cb = cb;
  _ep_data->cbdata = cbdata;
  _ep_data->_this = this;

  _thread_data = _ep_data;

  _thread_handle = CreateThread(
    NULL,
    0,
    _thread_entry_point,
    _ep_data,
    0,
    NULL
  );
#endif

  return true;
}

bool runtime_handler::is_currently_executing() const{
  if(!_thread_handle)
    return false;

#if (_WIN64) || (_WIN32)
  DWORD _exit_code;
  BOOL _ret = GetExitCodeThread(_thread_handle, &_exit_code);
  if(!_ret)
    return false;

  return _exit_code == STILL_ACTIVE;
#endif
}


int runtime_handler::run_current_file(){
  if(!_state)
    return -1;

  lua_getglobal(_state, LUD_RUNTIME_HANDLER_LUA_CHUNK_FUNC);
  int _err_code = lua_pcall(_state, 0, 0, 0);
  if(_err_code != LUA_OK)
    _read_error_obj();
  
  return _err_code;
}

int runtime_handler::load_file(const char* lua_path){
  if(!_state)
    return -1;

  int _err_code = luaL_loadfile(_state, lua_path);
  if(_err_code != LUA_OK){
    _read_error_obj();
    return _err_code;
  }

  lua_setglobal(_state, LUD_RUNTIME_HANDLER_LUA_CHUNK_FUNC);
  return LUA_OK;
}

void runtime_handler::load_std_libs(){
  if(!_state)
    return;

  luaL_openlibs(_state);
}


#if (_WIN64) || (_WIN32)
DWORD runtime_handler::get_running_thread_id(){
  return _thread_handle? GetThreadId(_thread_handle): 0;
}


void runtime_handler::register_event_execution_finished(HANDLE event){
  _event_finished.insert(event);
}

void runtime_handler::remove_event_execution_finished(HANDLE event){
  _event_finished.erase(event);
}
#endif


const lua::I_variant* runtime_handler::get_last_error_object(){
  return _last_err_obj;
}


void runtime_handler::set_logger(I_logger* logger){
  _logger = logger;

  _hook_handler->set_logger(logger);
  _execution_flow->set_logger(logger);
  _function_database->set_logger(logger);
  _library_loader->set_logger(logger);
}



// MARK: DLL functions

DLLEXPORT lua::I_runtime_handler* CPPLUA_CREATE_RUNTIME_HANDLER(const char* lua_path, bool immediate_run, bool load_library){
  if(lua_path)
    return new runtime_handler(lua_path, immediate_run, load_library);
  else
    return new runtime_handler(load_library);
}

DLLEXPORT void CPPLUA_DELETE_RUNTIME_HANDLER(I_runtime_handler* handler){
  delete handler;
}