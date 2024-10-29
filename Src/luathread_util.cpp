#include "dllutil.h"
#include "error_util.h"
#include "luaapi_internal.h"
#include "luaapi_stack.h"
#include "luaapi_value.h"
#include "luaI_object.h"
#include "luamemory_util.h"
#include "luaobject_util.h"
#include "luastate_metadata.h"
#include "luathread_util.h"
#include "luautility.h"
#include "luavariant.h"
#include "string_util.h"

#include "../LuaSrc/lstate.h"

#include "map"
#include "stdexcept"

#if (_WIN64) || (_WIN32)
#include "Windows.h"
#endif


#ifdef LUA_CODE_EXISTS

using namespace dynamic_library::util;
using namespace error::util;
using namespace lua;
using namespace lua::memory;
using namespace lua::object;
using namespace lua::state;
using namespace lua::thread;
using namespace lua::utility;

#define LUA_MUTEX_OBJECT_KEY "__cpplua_lua_mutex"

// NOTE: Even with the mutex stored in the internal storage, getting the mutex should not use lua_State as that could introduce infinite recursing.

struct __thread_state_data{
  lua_State* orig_state = NULL;

  bool enable = true;
};

typedef std::map<void*, __thread_state_data*> thread_data_mapping;

#if (_WIN64) || (_WIN32)
static DWORD _thread_data_tidx = TLS_OUT_OF_INDEXES;
#endif

static ::memory::I_dynamic_management* __dm = get_memory_manager();



static void _init_thread(){
  thread_data_mapping* _thread_map = (thread_data_mapping*)TlsGetValue(_thread_data_tidx);
  if(!_thread_map){
    _thread_map = __dm->new_class_dbg<thread_data_mapping>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
    TlsSetValue(_thread_data_tidx, _thread_map);
  }
}

static void _deinit_thread(){
  thread_data_mapping* _thread_map = (thread_data_mapping*)TlsGetValue(_thread_data_tidx);
  if(_thread_map){
    for(auto _pair: *_thread_map)
      __dm->delete_class_dbg(_pair.second, DYNAMIC_MANAGEMENT_DEBUG_DATA);

    delete _thread_map;
    TlsSetValue(_thread_data_tidx, NULL);
  }
}


#if (_WIN64) || (_WIN32)
BOOL __cpplua_thread_util_dllevent(HINSTANCE inst, DWORD reason, LPVOID){
  switch(reason){
    break; case DLL_PROCESS_ATTACH:{
      _thread_data_tidx = TlsAlloc();
      if(_thread_data_tidx == TLS_OUT_OF_INDEXES)
        return false;
    }

    break; case DLL_PROCESS_DETACH:{
      _deinit_thread();
      TlsFree(_thread_data_tidx);
      _thread_data_tidx = TLS_OUT_OF_INDEXES;
    }

    break; case DLL_THREAD_DETACH:{
      _deinit_thread();
    }
  }

  return true;
}
#endif


static void _def_lua_mutex_destructor(I_object* object){
  __dm->delete_class_dbg(object, DYNAMIC_MANAGEMENT_DEBUG_DATA);
}

static const fdata _lua_mutex_functions[] = {
  fdata(NULL, NULL)
};

class _lua_mutex: public I_mutex, public function_store, virtual public I_object{
#if (_WIN64) || (_WIN32)
  private:
    lua_State* _state;
    CRITICAL_SECTION _mutex;

  public:
    _lua_mutex(lua_State* state): function_store(_def_lua_mutex_destructor){
      set_function_data(_lua_mutex_functions);

      _state = state;
      InitializeCriticalSection(&_mutex);
    }

    ~_lua_mutex(){
      DeleteCriticalSection(&_mutex);
    }


    void lock() override{
      EnterCriticalSection(&_mutex);
    }

    void unlock() override{
      LeaveCriticalSection(&_mutex);
    }


    lua::I_debuggable_object* as_debug_object() override{
      return NULL;
    }

    void on_object_added(const lua::api::core* lc) override{

    }
#endif
};


I_mutex* lua::thread::require_state_mutex(lua_State* state){
  _lua_mutex* _result = NULL;

  I_metadata* _metadata = get_metadata(state);
  _metadata->lock_metadata();

  I_object* _mutex_obj = _metadata->get_object(LUA_MUTEX_OBJECT_KEY);
  if(_mutex_obj){
    _result = dynamic_cast<_lua_mutex*>(_result);
    goto skip_to_return;
  }

{ // enclosure for using goto
  _result = __dm->new_class_dbg<_lua_mutex>(DYNAMIC_MANAGEMENT_DEBUG_DATA, state);
  _metadata->set_object(LUA_MUTEX_OBJECT_KEY, _result);
} // enclosure closing

  skip_to_return:{}
  _metadata->unlock_metadata();
  return _result;
}


lua_State* lua::thread::require_dependent_state(lua_State* initiate_state){
  _init_thread();
  lua_State* _result = NULL;

  luathread_util_lock_state(initiate_state);
  
  thread_data_mapping* _thread = (thread_data_mapping*)TlsGetValue(_thread_data_tidx);

{ // enclosure for goto

  auto _iter = _thread->find(initiate_state->l_G->mainthread);
  if(_iter == _thread->end()){ // initiate thread
    _result = lua_newthread(initiate_state);

    __thread_state_data* _data = __dm->new_class_dbg<__thread_state_data>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _data->orig_state = _result;
    _data->enable = true;

    _thread->operator[](initiate_state->l_G->mainthread) = _data;
    goto skip_to_return;
  }

  __thread_state_data* _data = _iter->second;

  // if thread dependent state is temporarily disabled
  if(!_data->enable){
    _result = initiate_state;
    goto skip_to_return;
  }

  // return dependent thread
  _result = _data->orig_state;

} // enclosure closing


  skip_to_return:{}
  luathread_util_unlock_state(initiate_state);
  return _result;
}


void lua::thread::thread_dependent_enable(lua_State* lstate, bool enable){
  _init_thread();
  luathread_util_lock_state(lstate);

  thread_data_mapping* _thread = (thread_data_mapping*)TlsGetValue(_thread_data_tidx);
  auto _iter = _thread->find(lstate->l_G->mainthread);
  if(_iter == _thread->end()) // impossible to fail, already initiated at state creation
    goto skip_to_return;

  _iter->second->enable = enable;

  skip_to_return:{}
  luathread_util_unlock_state(lstate);
}


void lua::thread::lock_state(lua_State* lstate){
  I_metadata* _metadata = get_metadata(lstate);
  if(!_metadata)
    return;

  _metadata->lock_metadata();
  I_object* _mutex_obj = _metadata->get_object(LUA_MUTEX_OBJECT_KEY);
  I_mutex* _mutex = dynamic_cast<I_mutex*>(_mutex_obj);
  _metadata->unlock_metadata();

  if(!_mutex)
    return;
  
  _mutex->lock();
}

void lua::thread::unlock_state(lua_State* lstate){
  I_metadata* _metadata = get_metadata(lstate);
  if(!_metadata)
    return;
  
  _metadata->lock_metadata();
  I_object* _mutex_obj = _metadata->get_object(LUA_MUTEX_OBJECT_KEY);
  I_mutex* _mutex = dynamic_cast<I_mutex*>(_mutex_obj);
  _metadata->unlock_metadata();

  if(!_mutex)
    return;
  
  _mutex->unlock();
}


void luathread_util_lock_state(lua_State* lstate){
  lock_state(lstate);
}

void luathread_util_unlock_state(lua_State* lstate){
  unlock_state(lstate);
}


lua_State* luathread_util_get_tdependent(lua_State* lstate){
  return lua::thread::require_dependent_state(lstate);
}


void luathread_util_enable_tdependent(lua_State* lstate){
  lua::thread::thread_dependent_enable(lstate, true);
}

void luathread_util_disable_tdependent(lua_State* lstate){
  lua::thread::thread_dependent_enable(lstate, false);
}


#endif // LUA_CODE_EXISTS