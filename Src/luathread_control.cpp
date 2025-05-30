#include "dllutil.h"
#include "luaapi_core.h"
#include "luamemory_util.h"
#include "luaruntime_handler.h"
#include "luathread_control.h"
#include "luathread_util.h"
#include "memdynamic_management.h"
#include "std_logger.h"
#include "string_util.h"

#if (_WIN64) || (_WIN32)
#include "Windows.h"
#endif

#include "stdexcept"

#ifdef LUA_CODE_EXISTS

using namespace dynamic_library::util;
using namespace error::util;
using namespace lua;
using namespace lua::api;
using namespace lua::debug;
using namespace lua::memory;
using namespace lua::thread;
using namespace ::memory;




static I_dynamic_management* __dm = get_memory_manager();

thread_local thread_handle* _thandle_tls;



// MARK: thread_handle definition

#if (_WIN64) || (_WIN32)
thread_handle::thread_handle(HANDLE thread_handle, lua_State* lstate){
  _thread_handle = thread_handle;
  _current_tstate = lstate;

  _init_class(lstate);
}
#elif (__linux)
thread_handle::thread_handle(pthread_t thread_handle, lua_State* lstate){
  _thread_id = thread_handle;
  _current_tstate = lstate;

  _init_class(lstate);
}
#endif

thread_handle::~thread_handle(){
  __dm->delete_class_dbg(_exec_flow, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  __dm->delete_class_dbg(_hook, DYNAMIC_MANAGEMENT_DEBUG_DATA);
}


void thread_handle::_signal_stop(){
  _stop_thread = true;
}


void thread_handle::_hook_cb(lua_State* state){
  if(!_stop_thread)
    return;

  // throw an error that might be catched by pcall
  throw std::runtime_error("thread stopped");
}

void thread_handle::_hook_cb_static(const core* lc, void* cbdata){
  ((thread_handle*)cbdata)->_hook_cb((lua_State*)lc->istate);
}


void thread_handle::_init_class(lua_State* lstate){
  _stop_thread = false;
  _current_tstate = require_dependent_state(lstate);

  _hook = __dm->new_class_dbg<hook_handler>(DYNAMIC_MANAGEMENT_DEBUG_DATA, _current_tstate);
  _exec_flow = __dm->new_class_dbg<execution_flow>(DYNAMIC_MANAGEMENT_DEBUG_DATA, _hook);

  _hook->set_hook(LUA_MASKLINE, _hook_cb_static, this);
}


void thread_handle::stop_running(){
  _signal_stop();

  _exec_flow->resume_execution();
  join();
}

void thread_handle::signal_stop(){
  _signal_stop();
}


bool thread_handle::is_stop_signal() const{
  return _stop_thread;
}

bool thread_handle::is_stopped() const{
#if (_WIN64) || (_WIN32)
  DWORD _result = WaitForSingleObject(_thread_handle, 0);
  switch(_result){
    break;
    case WAIT_TIMEOUT:
    case WAIT_FAILED:
      return false;
  }

  return true;
#elif (__linux)
  return !_thread_id_valid;
#endif
}


void thread_handle::join(){
  try_join(INFINITE);
}

void thread_handle::try_join(unsigned long wait_ms){
#if (_WIN64) || (_WIN32)
  if(_thread_handle == INVALID_HANDLE_VALUE)
    return;

  DWORD _result = WaitForSingleObject(_thread_handle, wait_ms);
  if(_result != WAIT_OBJECT_0)
    return;
    
  CloseHandle(_thread_handle);
  _thread_handle = INVALID_HANDLE_VALUE;
#elif (__linux)
  if(!_thread_id_valid)
    return;

  timespec _ts;
    _ts.tv_sec = wait_ms/1000;
    _ts.tv_nsec = (wait_ms%1000)*1000000;

  int _result = pthread_timedjoin_np(_thread_id, NULL, &_ts);
  if(!_result)
    _thread_id_valid = false;
#endif
}

bool thread_handle::joinable(){
#if (_WIN64) || (_WIN32)
  if(_thread_handle == INVALID_HANDLE_VALUE)
    return false;

  DWORD _result = WaitForSingleObject(_thread_handle, 0);
  switch(_result){
    break;
    case WAIT_FAILED:
      return false;
  }

  return true;
#elif (__linux)
  return _thread_id_valid;
#endif
}

void thread_handle::detach(){
#if (_WIN64) || (_WIN32)
  if(_thread_handle == INVALID_HANDLE_VALUE)
    return;

  CloseHandle(_thread_handle);
  _thread_handle = INVALID_HANDLE_VALUE;
#elif (__linux)
  if(!_thread_id_valid)
    return;

  pthread_detach(_thread_id);
  _thread_id_valid = false;
#endif
}


unsigned long thread_handle::get_thread_id() const{
#if (_WIN64) || (_WIN32)
  return GetThreadId(_thread_handle);
#elif (__linux)
  return _thread_id;
#endif
}


void* thread_handle::get_lua_state_interface() const{
  return _current_tstate;
}

lua_State* thread_handle::get_lua_state() const{
  return _current_tstate;
}


I_execution_flow* thread_handle::get_execution_flow_interface() const{
  return _exec_flow;
}

I_hook_handler* thread_handle::get_hook_handler_interface() const{
  return _hook;
}



// MARK: thread_handle_reference definition

thread_handle_reference::thread_handle_reference(const thread_handle_reference* ref){
  _thread_ptr = ref->_thread_ptr;
}

thread_handle_reference::thread_handle_reference(thread_handle* handle, handle_destructor destructor){
  _thread_ptr = std::shared_ptr<thread_handle>(handle, destructor);
}

thread_handle_reference::~thread_handle_reference(){

}


thread_handle* thread_handle_reference::get() const{
  return _thread_ptr.get();
}

I_thread_handle* thread_handle_reference::get_interface() const{
  return _thread_ptr.get();
}


// TODO check for join, just in case

// MARK: thread_control definition

thread_control::thread_control(runtime_handler* rh){
  _object_mutex_ptr = &_object_mutex;

  _rh = rh;
  _state = rh->get_lua_state();
  _logger = get_std_logger();
}

thread_control::~thread_control(){
  _stop_all_execution();
}


#if (_WIN64) || (_WIN32)
DWORD WINAPI thread_control::__thread_entry_point(LPVOID data){
  _t_entry_point_data* _pointdata = (_t_entry_point_data*)data;
  __thread_entry_point_generalize(_pointdata);

  __dm->delete_class_dbg(_pointdata, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  return 0;
}
#elif (__linux)
void* thread_control::__thread_entry_point(void* data){
  _t_entry_point_data* _pointdata = (_t_entry_point_data*)data;
  __thread_entry_point_generalize(_pointdata);

  __dm->delete_class_dbg(_pointdata, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  return NULL;
}
#endif


void thread_control::__thread_entry_point_generalize(_t_entry_point_data* data){
{ // enclosure for mutex
  std::lock_guard _lg_init(data->init_mutex);

  thread_control* _this = data->_this;
  unsigned long _thread_id = data->thread_handle;

  thread_handle* _thread_handle = __dm->new_class_dbg<thread_handle>(DYNAMIC_MANAGEMENT_DEBUG_DATA, data->thread_handle, data->lstate);
  _thandle_tls = _thread_handle;

{ // enclosure for mutex 
  std::lock_guard _lg_obj(_this->_object_mutex);
  _this->_thread_data[_thread_id] = __dm->new_class_dbg<thread_handle_reference>(
    DYNAMIC_MANAGEMENT_DEBUG_DATA,
    _thread_handle,
    [](thread_handle* obj){
      __dm->delete_class_dbg(obj, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    }
  );
} // enclosure closing

  data->init_cond.notify_all();
} // enclosure closing

  data->cb(data->cbdata);
}


void thread_control::_stop_all_execution(){
  std::lock_guard _lg(_object_mutex);
  
  // signalling all threads to stop
  for(auto _pair: _thread_data){
    thread_handle* _handle_object = _pair.second->get();
    _handle_object->signal_stop();
  }

  // join all
  for(auto _pair: _thread_data){
    thread_handle* _handle_object = _pair.second->get();
    _handle_object->join();
  }
  
  // delete all thread objects
  while(_thread_data.size() > 0){
    auto _iter = _thread_data.begin();
    __dm->delete_class_dbg(_iter->second, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _thread_data.erase(_iter);
  }
}


unsigned long thread_control::run_execution(execution_context cb, void* cbdata){
  // Note: lifetime, whilst the thread is still alive
  _t_entry_point_data* _tepdata = __dm->new_class_dbg<_t_entry_point_data>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _tepdata->cb = cb;
    _tepdata->cbdata = cbdata;
    _tepdata->lstate = _state;
    _tepdata->_this = this;

  unsigned long _thread_id = ERROR_INVALID_THREAD_ID;

{ // enclosure for mutex
  std::unique_lock _ul(_tepdata->init_mutex);

#if (_WIN64) || (_WIN32)
  HANDLE _thread_handle = CreateThread(NULL, 0, thread_control::__thread_entry_point, _tepdata, 0, &_thread_id);
  if(!_thread_handle){
    if(_logger)
      _logger->print_error(format_str("[thread_control] Cannot create thread: %s\n", get_windows_error_message(GetLastError()).c_str()).c_str());

    __dm->free(_tepdata, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    return 0;
  }

  _tepdata->thread_handle = _thread_handle;
#elif (__linux)
  pthread_t _thread_id_pt;
  int _err_code = pthread_create(&_thread_id_pt, NULL, thread_control::__thread_entry_point, _tepdata);
  if(_err_code != 0){
    if(_logger)
      _logger->print_error(format_str("[thread_control] Cannot creat thread: errcode %d\n", _err_code).c_str());

    __dm->free(_tepdata, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _thread_id = _thread_id_pt;
    return 0;
  }

  _tepdata->thread_handle = _thread_id_pt;
#endif

  _tepdata->init_cond.wait(_ul);
} // enclosure closing

  skip_to_return:{}
  return _thread_id;
}


I_thread_handle_reference* thread_control::get_thread_handle(unsigned long thread_id){
  std::lock_guard _lg(_object_mutex);
  auto _iter = _thread_data.find(thread_id);
  if(_iter == _thread_data.end())
    return NULL;

  thread_handle* _handle = _iter->second->get();
  if(_handle->is_stopped()){
    __dm->delete_class_dbg(_iter->second, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _thread_data.erase(_iter);
    return NULL;
  }

  return __dm->new_class_dbg<thread_handle_reference>(DYNAMIC_MANAGEMENT_DEBUG_DATA, _iter->second);
}

void thread_control::free_thread_handle(I_thread_handle_reference* ref){
  if(!ref)
    return;

  __dm->delete_class_dbg(ref, DYNAMIC_MANAGEMENT_DEBUG_DATA);
}


void thread_control::set_logger(I_logger* logger){
  _logger = logger;
}



thread_handle* lua::get_thread_handle(){
  return _thandle_tls;
}

#endif // LUA_CODE_EXISTS