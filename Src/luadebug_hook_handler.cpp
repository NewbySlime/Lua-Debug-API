#include "dllutil.h"
#include "luaapi_core.h"
#include "luadebug_hook_handler.h"
#include "luainternal_storage.h"
#include "luamemory_util.h"
#include "luautility.h"
#include "luavariant.h"
#include "luavariant_util.h"
#include "std_logger.h"
#include "string_util.h"

#include "map"

#if (_WIN64) || (_WIN32)
#include "Windows.h"
#endif

#define LUA_MAX_MASK_COUNT (LUA_HOOKCOUNT + 1)


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
static std::map<void*, hook_handler*>* _hook_lists;

#if (_WIN64) || (_WIN32)
static CRITICAL_SECTION* _code_mutex;  
#endif

static void _code_initiate();
static void _code_deinitiate();
static destructor_helper _dh(_code_deinitiate);


static void _code_initiate(){
#if (_WIN64) || (_WIN32)
  if(!_code_mutex){
    _code_mutex = (CRITICAL_SECTION*)__dm->malloc(sizeof(CRITICAL_SECTION), DYNAMIC_MANAGEMENT_DEBUG_DATA);
    InitializeCriticalSection(_code_mutex);
  }
#endif

  if(!_hook_lists)
    _hook_lists = __dm->new_class_dbg<std::map<void*, hook_handler*>>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
}

static void _code_deinitiate(){
  if(_hook_lists){
    __dm->delete_class_dbg(_hook_lists, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _hook_lists = NULL;
  }

#if (_WIN64) || (_WIN32)
  if(_code_mutex){
    DeleteCriticalSection(_code_mutex);
    __dm->free(_code_mutex, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _code_mutex = NULL;
  }
#endif
}


static void _lock_code(){
#if (_WIN64) || (_WIN32)
  EnterCriticalSection(_code_mutex);
#endif
}

static void _unlock_code(){
#if (_WIN64) || (_WIN32)
  LeaveCriticalSection(_code_mutex);
#endif
}


// MARK: lua::hook_handler
hook_handler::hook_handler(lua_State* state, int count){
  _code_initiate();

#if (_WIN64) || (_WIN32)
  _object_mutex_ptr = &_object_mutex;
  InitializeCriticalSection(_object_mutex_ptr);
#endif

  _logger = get_std_logger();
  _this_state = state;
  this->count = count;
  _update_hook_config();

  _lock_code();
  _hook_lists->operator[](state) = this;
  _unlock_code();
}

hook_handler::~hook_handler(){
  _lock_code();
  _hook_lists->erase(_this_state);
  _unlock_code();
  
  // clear data resetting sethook
  _lock_object();
  _call_hook_set.clear();
  _return_hook_set.clear();
  _line_hook_set.clear();
  _count_hook_set.clear();
  _update_hook_config();
  _unlock_object();

#if (_WIN64) || (_WIN32)
  DeleteCriticalSection(_object_mutex_ptr);
#endif
}


void hook_handler::_lock_object() const{
#if (_WIN64) || (_WIN32)
  EnterCriticalSection(_object_mutex_ptr);
#endif
}

void hook_handler::_unlock_object() const{
#if (_WIN64) || (_WIN32)
  LeaveCriticalSection(_object_mutex_ptr);
#endif
}


void hook_handler::_update_hook_config(){
  lua_sethook(_this_state, _on_hook_event_static, 
    LUA_MASKCALL * (_call_hook_set.size() > 0) |
    LUA_MASKRET * (_return_hook_set.size() > 0) |
    LUA_MASKLINE * (_line_hook_set.size() > 0) |
    LUA_MASKCOUNT * (_count_hook_set.size() > 0),
  count);
}

void hook_handler::_on_hook_event(lua_State* state, lua_Debug* dbg){
  _current_dbg = dbg;
  std::map<void*, hookcb>* _current_hook_map = NULL;

  bool _error_occured = false;
  std::exception _exception_data;

  lua_getinfo(state, "nSl", _current_dbg);
  switch(dbg->event){
    break; case LUA_HOOKTAILCALL:
     case LUA_HOOKCALL:{
      _current_hook_map = &_call_hook_set;
    }

    break; case LUA_HOOKRET:{
      _current_hook_map = &_return_hook_set;
    }

    break; case LUA_HOOKLINE:{
      _current_hook_map = &_line_hook_set;
    }

    break; case LUA_HOOKCOUNT:{
      _current_hook_map = &_count_hook_set;
    }
  }

  core lc = as_lua_api_core(state);
  
  _lock_object();
  for(auto _pair: *_current_hook_map){
    try{
      _pair.second(&lc, _pair.first);
    }
    catch(std::exception e){
      _error_occured = true;
      _exception_data = e;
    }
  }
  _unlock_object();

  _current_dbg = NULL;
  if(_error_occured){
    lua_pushstring(_this_state, _exception_data.what());
    lua_error(_this_state);
  }
}


void hook_handler::_on_hook_event_static(lua_State* state, lua_Debug* dbg){
  if(!_hook_lists)
    return;
  
  _lock_code();
  hook_handler* _hook = NULL;
{ // enclosure for using gotos
  auto _iter = _hook_lists->find(state);
  if(_iter == _hook_lists->end())
    goto skip_to_continue;

  _hook = _iter->second;
} // enclosure closing

  skip_to_continue:{}
  _unlock_code();

  if(!_hook)
    return;
  
  _hook->_on_hook_event(state, dbg);
}


void hook_handler::set_hook(int hook_mask, hook_handler::hookcb cb, void* attached_obj){
  _lock_object();
  remove_hook(attached_obj);
  for(int i = 0; i < LUA_MAX_MASK_COUNT; i++){
    int _current_mask = 1 << i;
    if(!(_current_mask & hook_mask))
      continue;

    switch(_current_mask){
      break; case LUA_MASKCALL:{
        _call_hook_set[attached_obj] = cb;
      }

      break; case LUA_MASKRET:{
        _return_hook_set[attached_obj] = cb;
      }

      break; case LUA_MASKLINE:{
        _line_hook_set[attached_obj] = cb;
      }

      break; case LUA_MASKCOUNT:{
        _count_hook_set[attached_obj] = cb;
      }
    }
  }

  _update_hook_config();
  _unlock_object();
}

void hook_handler::remove_hook(void* attached_obj){
  _lock_object();
  _call_hook_set.erase(attached_obj);
  _return_hook_set.erase(attached_obj);
  _line_hook_set.erase(attached_obj);
  _count_hook_set.erase(attached_obj);

  _update_hook_config();
  _unlock_object();
}


void hook_handler::set_count(int count){
  _lock_object();
  this->count = count;
  _update_hook_config();
  _unlock_object();
}

int hook_handler::get_count() const{
  return count;
}


void* hook_handler::get_lua_state_interface() const{
  return _this_state;
}

lua_State* hook_handler::get_lua_state() const{
  return _this_state;
}


const lua_Debug* hook_handler::get_current_debug_value() const{
  return _current_dbg;
}


void hook_handler::set_logger(I_logger* logger){
  _logger = logger;
}

#endif // LUA_CODE_EXISTS