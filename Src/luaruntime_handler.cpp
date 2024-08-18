#include "lua_variant.h"
#include "luaruntime_handler.h"
#include "stdlogger.h"

#define LUD_RUNTIME_HANDLER_VAR_NAME "__clua_runtime_handler"


using namespace lua;
using namespace lua::debug;



runtime_handler::runtime_handler(lua_State* state){
  _create_own_lua_state = false;

  _logger = get_stdlogger();
  _state = NULL;

#if _WIN64
  _thread_handle = NULL;
#endif

  runtime_handler* _current_handler = get_attached_object(state);
  if(_current_handler){
    _logger->print_error("Cannot create runtime_handler. Reason: lua_State already has runtime_handler.\n");
    return;
  }

  _state = state;
  _initiate_class();
}

runtime_handler::runtime_handler(const std::string& lua_path){
  _create_own_lua_state = true;

  _logger = get_stdlogger();
  _state = NULL;

#if _WIN64
  _thread_handle = NULL;
#endif

  lua_State* _created_state = luaL_newstate();
  luaL_openlibs(_created_state);

  int _res = luaL_dofile(_created_state, lua_path.c_str());
  if(_res != LUA_OK){
    lua::variant* _var = lua::to_variant(_created_state, -1);
    std::string _err_str = _var->to_string();

    delete _var;
    _logger->print(format_str("Cannot open lua file '%s' Error: %s\n", lua_path.c_str(), _err_str.c_str()));

    lua_close(_created_state);
    return;
  }

  _state = _created_state;
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

  if(_create_own_lua_state)
    lua_close(_state);
}


#if _WIN64
DWORD runtime_handler::_thread_entry_point(LPVOID data){
  _t_entry_point_data* _ep_data = (_t_entry_point_data*)data;

  execution_context _context = _ep_data->cb;
  void* _context_data = _ep_data->cbdata;

  delete _ep_data;

  _context(_context_data);
  return 0;
}
#endif


void runtime_handler::_initiate_class(){
  _set_bind_obj(this, _state);

  _hook_handler = new hook_handler(_state);
  _hook_handler->set_hook(LUA_MASKLINE, _hookcb_static, this);

  _execution_flow = new execution_flow(_state);

  _function_database = new func_db(_state);
}


void runtime_handler::_hookcb(){
  const lua_Debug* _debug_data = _hook_handler->get_current_debug_value();
  if(_debug_data->event != LUA_HOOKLINE)
    return;

#if _WIN64
  if(!_stop_thread || GetThreadId(_thread_handle) != GetCurrentThreadId())
    return;

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


func_db* runtime_handler::get_function_database(){
  return _function_database;
}

lua_State* runtime_handler::get_lua_state(){
  return _state;
}


hook_handler* runtime_handler::get_hook_handler(){
  return _hook_handler;
}

execution_flow* runtime_handler::get_execution_flow(){
  return _execution_flow;
}


void runtime_handler::stop_execution(){
  if(!_thread_handle)
    return;

#if _WIN64
  if(is_currently_executing()){
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

#if _WIN64
    _t_entry_point_data* _ep_data = new _t_entry_point_data();
    _ep_data->cb = cb;
    _ep_data->cbdata = cbdata;

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

bool runtime_handler::is_currently_executing(){
  if(!_thread_handle)
    return false;

#if _WIN64
  DWORD _exit_code;
  BOOL _ret = GetExitCodeThread(_thread_handle, &_exit_code);
  if(!_ret)
    return false;

  return _exit_code == STILL_ACTIVE;
#endif
}


void runtime_handler::set_logger(I_logger* logger){
  _logger = logger;
}