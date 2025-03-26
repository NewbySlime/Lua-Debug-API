#include "dllruntime.h"
#include "dllutil.h"
#include "error_util.h"
#include "luaapi_internal.h"
#include "luaapi_stack.h"
#include "luaapi_value.h"
#include "luaI_object.h"
#include "luainternal_storage.h"
#include "luamemory_util.h"
#include "luaobject_util.h"
#include "luastate_metadata.h"
#include "luathread_util.h"
#include "luautility.h"
#include "luavariant.h"
#include "string_util.h"

#include "../LuaSrc/lstate.h"

#include "map"
#include "mutex"
#include "stdexcept"
#include "thread"

#ifdef LUA_CODE_EXISTS

using namespace dynamic_library::util;
using namespace error::util;
using namespace lua;
using namespace lua::internal;
using namespace lua::memory;
using namespace lua::object;
using namespace lua::state;
using namespace lua::thread;
using namespace lua::utility;

#define LUA_MUTEX_OBJECT_KEY "__cpplua_lua_mutex"
#define LUA_THREAD_METASTORAGE "__cpplua_thread"

// NOTE: Even with the mutex stored in the internal storage, getting the mutex should not use lua_State as that could introduce infinite recursing.

struct __thread_state_data{
  lua_State* orig_state = NULL;

  bool enable = true;
};

typedef std::map<void*, __thread_state_data*> state_data_mapping;
typedef std::map<std::thread::id, state_data_mapping*> thread_data_mapping;

static ::memory::I_dynamic_management* __dm = get_memory_manager();

static thread_data_mapping* _thread_data = NULL;


static std::recursive_mutex* _code_mutex = NULL;

static void _code_initiate();
static void _code_deinitiate();
static destructor_helper _dh(_code_deinitiate);


static void _code_initiate(){
  if(!_thread_data)
    _thread_data = __dm->new_class_dbg<thread_data_mapping>(DYNAMIC_MANAGEMENT_DEBUG_DATA);

  if(!_code_mutex)
    _code_mutex = __dm->new_class_dbg<std::recursive_mutex>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
}

static void _code_deinitiate(){
  if(_thread_data){
    std::lock_guard _lg(*_code_mutex);
    for(auto tpair: *_thread_data){
      for(auto spair: *tpair.second)
        __dm->delete_class_dbg(spair.second, DYNAMIC_MANAGEMENT_DEBUG_DATA);

      __dm->delete_class_dbg(tpair.second, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    }

    __dm->delete_class_dbg(_thread_data, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _thread_data = NULL;
  }

  if(_code_mutex){
    __dm->delete_class_dbg(_code_mutex, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _code_mutex = NULL;
  }
}


static void _require_internal_data(lua_State* state){
  // internal storage s+1
  require_internal_storage(state);

  lua_pushstring(state, LUA_THREAD_METASTORAGE);
  if(lua_gettable(state, -2) != LUA_TTABLE){
    lua_pop(state, 1); // pop the last value first

    lua_newtable(state); // don't pop, will be the result

    lua_pushstring(state, LUA_THREAD_METASTORAGE);
    lua_pushvalue(state, -2);
    lua_settable(state, -4);
  }

  lua_remove(state, -2); // remove the internal storage
}

static void _register_thread_to_internal(lua_State* key, lua_State* tstate){
  _require_internal_data(tstate);

  std::string _pkey = format_str("0x%X", key);
  lua_pushstring(tstate, _pkey.c_str());
  lua_pushthread(tstate);
  lua_settable(tstate, -3);

  // pop internal storage
  lua_pop(tstate, 1);
}


static void _init_thread(){
  _code_initiate();
  std::thread::id _tid = std::this_thread::get_id();

  std::lock_guard _lg(*_code_mutex);
  auto _iter = _thread_data->find(_tid);
  if(_iter == _thread_data->end()){
    state_data_mapping* _state_mapping = __dm->new_class_dbg<state_data_mapping>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _thread_data->operator[](_tid) = _state_mapping;
  }
}

// Called when a thread is stopped
static void _deinit_thread(){
  if(!_thread_data)
    return;

  std::thread::id _tid = std::this_thread::get_id();

  std::lock_guard _lg(*_code_mutex);
  auto _iter = _thread_data->find(_tid);
  if(_iter != _thread_data->end()){
    for(auto spair: *_iter->second){
      lua_State* _main_thread = (lua_State*)spair.first;
      _register_thread_to_internal(_main_thread, spair.second->orig_state);

      // remove thread dependents since the thread is stopping
      __dm->delete_class_dbg(spair.second, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    }
  
    // remove thread data
    __dm->delete_class_dbg(_iter->second, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _thread_data->erase(_iter);
  }
}


#if (_WIN64) || (_WIN32)
BOOL __cpplua_thread_util_dllevent_win(HINSTANCE inst, DWORD reason, LPVOID){
  switch(reason){
    break; case DLL_THREAD_DETACH:{
      _deinit_thread();
    }
  }

  return true;
}
#else
void __cpplua_thread_util_dllevent_def(uint32_t state){
  switch(state){
    break; case def_dllevent_thread_stop:{
      _deinit_thread();
    }
  }
}
#endif


static void _def_lua_mutex_destructor(I_object* object){
  __dm->delete_class_dbg(object, DYNAMIC_MANAGEMENT_DEBUG_DATA);
}

static const fdata _lua_mutex_functions[] = {
  fdata(NULL, NULL)
};

class _lua_mutex: public I_mutex, public function_store, virtual public I_object{
  private:
    lua_State* _state;
#if (_WIN64) || (_WIN32)
    HANDLE _object_mutex;
#elif (__linux)
    pthread_mutex_t _object_mutex = PTHREAD_MUTEX_INTIIALIZER;
#endif

  public:
    _lua_mutex(lua_State* state): function_store(_def_lua_mutex_destructor){
      set_function_data(_lua_mutex_functions);

#if (_WIN64) || (_WIN32)
      _object_mutex = CreateMutex(NULL, false, NULL);
#endif
      _state = state;
    }

    ~_lua_mutex(){
#if (_WIN64) || (_WIN32)
      CloseHandle(_object_mutex);
#endif
    }


    void lock() override{
#if (_WIN64) || (_WIN32)
      WaitForSingleObject(_object_mutex, INFINITE);
#elif (__linux)
      pthread_mutex_lock(_object_mutex);
#endif
    }

    void unlock() override{
#if (_WIN64) || (_WIN32)
      ReleaseMutex(_object_mutex);
#elif (__linux)
      pthread_mutex_unlock(_object_mutex);
#endif
    }


    lua::I_debuggable_object* as_debug_object() override{
      return NULL;
    }

    void on_object_added(const lua::api::core* lc) override{

    }
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
  std::thread::id _tid = std::this_thread::get_id();

  std::lock_guard _lg(*_code_mutex);
  state_data_mapping* _thread = _thread_data->operator[](_tid);
  auto _iter = _thread->find(initiate_state->l_G->mainthread);
  if(_iter == _thread->end()){ // initiate thread
    lua_State* _result = lua_newthread(initiate_state);

    __thread_state_data* _data = __dm->new_class_dbg<__thread_state_data>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _data->orig_state = _result;
    _data->enable = false;

    _thread->operator[](initiate_state->l_G->mainthread) = _data;

    lua_pop(initiate_state, 1);
    _data->enable = true;

    _register_thread_to_internal(_result, _result);
    return _result;
  }

  __thread_state_data* _data = _iter->second;

  // if thread dependent state is temporarily disabled
  if(!_data->enable)
    return initiate_state;

  // return dependent thread
  return _data->orig_state;
}

void lua::thread::reset_dependent_state(lua_State* tstate, bool is_mainthread){
  if(!_thread_data)
    return;

  std::lock_guard _lg(*_code_mutex);
  if(!is_mainthread)
    tstate = tstate->l_G->mainthread;

  for(auto tpair: *_thread_data){
    auto _iter = tpair.second->find(tstate);
    if(_iter != tpair.second->end()){
      // remove thread dependent based on the deleted main thread
      __dm->delete_class_dbg(_iter->second, DYNAMIC_MANAGEMENT_DEBUG_DATA);
      tpair.second->erase(_iter);
    }
  }
}


void lua::thread::thread_dependent_enable(lua_State* lstate, bool enable){
  _init_thread();
  require_dependent_state(lstate);

  std::thread::id _tid = std::this_thread::get_id();

  std::lock_guard _lg(*_code_mutex);
  state_data_mapping* _thread = _thread_data->operator[](_tid);
  _thread->operator[](lstate->l_G->mainthread)->enable = enable;
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