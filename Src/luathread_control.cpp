#include "dllutil.h"
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


#ifdef LUA_CODE_EXISTS

using namespace dynamic_library::util;
using namespace error::util;
using namespace lua;
using namespace lua::debug;
using namespace lua::memory;
using namespace lua::thread;
using namespace ::memory;




static I_dynamic_management* __dm = get_memory_manager();


#if (_WIN64) || (_WIN32)
static DWORD _thandle_tidx = TLS_OUT_OF_INDEXES;
#endif


#if (_WIN64) || (_WIN32)
BOOL __cpplua_thread_control_dllevent(HINSTANCE inst, DWORD reason, LPVOID){
  switch(reason){
    break; case DLL_PROCESS_ATTACH:{
      _thandle_tidx = TlsAlloc();
      if(_thandle_tidx == TLS_OUT_OF_INDEXES)
        return false;
    }

    break; case DLL_PROCESS_DETACH:{
      TlsFree(_thandle_tidx);
      _thandle_tidx = TLS_OUT_OF_INDEXES;
    }
  }

  return true;
}
#endif




// MARK: thread_handle definition

#if (_WIN64) || (_WIN32)
thread_handle::thread_handle(HANDLE thread_handle, lua_State* lstate){
  InitializeCriticalSection(&_object_mutex);
  _stop_thread = false;
  _thread_handle = thread_handle;
  _suspend_count = 0;
  _current_tstate = require_dependent_state(lstate);

  _hook = __dm->new_class_dbg<hook_handler>(DYNAMIC_MANAGEMENT_DEBUG_DATA, _current_tstate);
  _exec_flow = __dm->new_class_dbg<execution_flow>(DYNAMIC_MANAGEMENT_DEBUG_DATA, _hook);

  _hook->set_hook(LUA_MASKLINE, _hook_cb_static, this);
}
#endif

thread_handle::~thread_handle(){
  __dm->delete_class_dbg(_exec_flow, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  __dm->delete_class_dbg(_hook, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  DeleteCriticalSection(&_object_mutex);

  // self close
  CloseHandle(_thread_handle);
}


void thread_handle::_lock_object(){
#if (_WIN64) || (_WIN32)
  EnterCriticalSection(&_object_mutex);
#endif
}

void thread_handle::_unlock_object(){
#if (_WIN64) || (_WIN32)
  LeaveCriticalSection(&_object_mutex);
#endif
}


void thread_handle::_signal_stop(){
  _stop_thread = true;
}

void thread_handle::_wait_for_thread_stop(){
  if(is_stopped())
    return;

#if (_WIN64) || (_WIN32)
  // skips if waiting same thread as current
  if(GetThreadId(_thread_handle) == GetCurrentThreadId())
    return;
    
  WaitForSingleObject(_thread_handle, INFINITE);
#endif
}


void thread_handle::_hook_cb(lua_State* state){
  if(!_stop_thread)
    return;

  // throw an error that might be catched by pcall
  throw std::runtime_error("thread stopped");
}

void thread_handle::_hook_cb_static(lua_State* state, void* cbdata){
  ((thread_handle*)cbdata)->_hook_cb(state);
}


void thread_handle::stop_running(){
  _signal_stop();

  _exec_flow->resume_execution();
  _wait_for_thread_stop();
}

void thread_handle::signal_stop(){
  _signal_stop();
}

bool thread_handle::is_stop_signal() const{
  return _stop_thread;
}


void thread_handle::wait_for_thread_stop(){
  _wait_for_thread_stop();
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
#endif
}


void thread_handle::pause(){
  if(is_stopped())
    return;

  _suspend_count = SuspendThread(_thread_handle)+1;
}

void thread_handle::resume(){
  if(!is_stopped())
    return;

  _suspend_count = ResumeThread(_thread_handle)-1;
}

bool thread_handle::is_paused(){
  if(is_stopped())
    return true;

  return _suspend_count > 0;
}


unsigned long thread_handle::get_thread_id() const{
  return GetThreadId(_thread_handle);
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



// MARK: thread_control definition

thread_control::thread_control(runtime_handler* rh){
#if (_WIN64) || (_WIN32)
  _object_mutex_ptr = &_object_mutex;
  InitializeCriticalSection(_object_mutex_ptr);
#endif

  _rh = rh;
  _state = rh->get_lua_state();
  _logger = get_std_logger();
}

thread_control::~thread_control(){
  _stop_all_execution();

#if (_WIN64) || (_WIN32)
  DeleteCriticalSection(_object_mutex_ptr);
#endif
}


#if (_WIN64) || (_WIN32)
DWORD WINAPI thread_control::__thread_entry_point(LPVOID data){
  _t_entry_point_data _copydata;{
    _copydata = *(_t_entry_point_data*)data;
    __dm->free(data, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  }

  thread_control* _this = _copydata._this;
  unsigned long _thread_id = GetCurrentThreadId();

  thread_handle* _thread_handle = __dm->new_class_dbg<thread_handle>(DYNAMIC_MANAGEMENT_DEBUG_DATA, _copydata.thread_handle, _copydata.lstate);
  TlsSetValue(_thandle_tidx, _thread_handle);

  _this->_lock_object();
  _this->_thread_data[_thread_id] = __dm->new_class_dbg<thread_handle_reference>(
    DYNAMIC_MANAGEMENT_DEBUG_DATA,
    _thread_handle,
    [](thread_handle* obj){
    __dm->delete_class_dbg(obj, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    }
  );
  _this->_unlock_object();

  SetEvent(_copydata.init_event);

  _copydata.cb(_copydata.cbdata);
  return 0;
}
#endif


void thread_control::_lock_object() const{
#if (_WIN64) || (_WIN32)
  EnterCriticalSection(_object_mutex_ptr);
#endif
}

void thread_control::_unlock_object() const{
#if (_WIN64) || (_WIN32)
  LeaveCriticalSection(_object_mutex_ptr);
#endif
}


void thread_control::_stop_all_execution(){
#if (_WIN64) || (_WIN32)
  _lock_object();
  size_t _handle_count = _thread_data.size();
  HANDLE* _handle_list = (HANDLE*)__dm->malloc(sizeof(HANDLE) * _handle_count, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  
  int _idx = 0;
  // signalling all thread to stop
  for(auto _pair: _thread_data){
    thread_handle* _handle_object = _pair.second->get();
    _handle_object->signal_stop();
    _handle_list[_idx] = _handle_object->_thread_handle;

    _idx++;
  }

  WaitForMultipleObjects(_handle_count, _handle_list, true, INFINITE);
  
  // delete all thread object
  while(_thread_data.size() > 0){
    auto _iter = _thread_data.begin();
    __dm->delete_class_dbg(_iter->second, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _thread_data.erase(_iter);
  }

  _unlock_object();

  __dm->free(_handle_list, DYNAMIC_MANAGEMENT_DEBUG_DATA);
#endif
}


unsigned long thread_control::run_execution(execution_context cb, void* cbdata){
  _t_entry_point_data* _tepdata = (_t_entry_point_data*)__dm->malloc(sizeof(_t_entry_point_data), DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _tepdata->cb = cb;
    _tepdata->cbdata = cbdata;
    _tepdata->lstate = _state;
    _tepdata->_this = this;

  unsigned long _thread_id = ERROR_INVALID_THREAD_ID;

#if (_WIN64) || (_WIN32)
  HANDLE _init_event = CreateEvent(NULL, true, false, NULL);
  _tepdata->init_event = _init_event;

  HANDLE _thread_handle = CreateThread(NULL, 0, thread_control::__thread_entry_point, _tepdata, CREATE_SUSPENDED, &_thread_id);
  if(!_thread_handle){
    if(_logger)
      _logger->print_error(format_str("[thread_control] Cannot create thread: %s\n", get_windows_error_message(GetLastError()).c_str()).c_str());

    __dm->free(_tepdata, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _thread_id = ERROR_INVALID_THREAD_ID;
    goto skip_to_return_winapi;
  }

  _tepdata->thread_handle = _thread_handle;
  ResumeThread(_thread_handle);
  
  WaitForSingleObject(_init_event, INFINITE);

  skip_to_return_winapi:{}
  CloseHandle(_init_event);
#endif

  return _thread_id;
}


I_thread_handle_reference* thread_control::get_thread_handle(unsigned long thread_id){
  I_thread_handle_reference* _result = NULL;
  _lock_object();
{ // enclosure for using gotos
  auto _iter = _thread_data.find(thread_id);
  if(_iter == _thread_data.end())
    goto skip_to_return;

  thread_handle* _handle = _iter->second->get();
  if(_handle->is_stopped()){
    __dm->delete_class_dbg(_iter->second, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _thread_data.erase(_iter);
    goto skip_to_return;
  }
  
  _result = __dm->new_class_dbg<thread_handle_reference>(DYNAMIC_MANAGEMENT_DEBUG_DATA, _iter->second);
} // enclosure closing

  skip_to_return:{}
  _unlock_object();

  return _result;
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
  return (thread_handle*)TlsGetValue(_thandle_tidx);
}

#endif // LUA_CODE_EXISTS