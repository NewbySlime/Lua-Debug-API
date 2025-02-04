#include "luadebug_execution_flow.h"
#include "luainternal_storage.h"
#include "luamemory_util.h"
#include "luastack_iter.h"
#include "luatable_util.h"
#include "luautility.h"
#include "luaruntime_handler.h"
#include "luavariant_util.h"
#include "std_logger.h"
#include "string_util.h"


using namespace lua;
using namespace lua::api;
using namespace lua::debug;
using namespace lua::internal;
using namespace lua::memory;
using namespace lua::utility;
using namespace ::memory;


#define FILE_PATH_MAX_BUFFER_SIZE 1024

#define FNAME_TAILCALLED_FLAG " (tailcalled)"


#ifdef LUA_CODE_EXISTS

static const I_dynamic_management* __dm = get_memory_manager();


// MARK: lua::debug::execution_flow
execution_flow::execution_flow(hook_handler* hook){
  _logger = get_std_logger();

#if (_WIN64) || (_WIN32)
  _object_mutex_ptr = &_object_mutex;
  InitializeCriticalSection(_object_mutex_ptr);
  _execution_block_event = CreateEvent(NULL, false, false, NULL);
  _self_pause_event = CreateEvent(NULL, false, false, NULL);
  _self_resume_event = CreateEvent(NULL, false, false, NULL);

  register_event_pausing(_self_pause_event);
  register_event_resuming(_self_resume_event);
#endif

  _hook_handler = hook;
  _hook_handler->set_hook(LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE | LUA_MASKCOUNT, _hookcb_static, this);
  
  _current_file_path.reserve(FILE_PATH_MAX_BUFFER_SIZE);
}

execution_flow::~execution_flow(){
  clear_breakpoints();

  _hook_handler->remove_hook(this);
  while(_function_layer.size() > 0){
    auto _iter = _function_layer.end()-1;
    _function_data* _data = *_iter;
    
    _function_layer.erase(_iter);
    __dm->delete_class_dbg(_data, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  }

  if(currently_pausing())
    resume_execution();

#if (_WIN64) || (_WIN32)
  remove_event_pausing(_self_pause_event);
  remove_event_resuming(_self_resume_event);

  CloseHandle(_self_pause_event);
  CloseHandle(_self_resume_event);
  CloseHandle(_execution_block_event);
  DeleteCriticalSection(_object_mutex_ptr);
#endif
}


void execution_flow::_lock_object() const{
#if (_WIN64) || (_WIN32)
  EnterCriticalSection(_object_mutex_ptr);
#endif
}

void execution_flow::_unlock_object() const{
#if (_WIN64) || (_WIN32)
  LeaveCriticalSection(_object_mutex_ptr);
#endif
}


void execution_flow::_update_tmp_current_fname(){
  _lock_object();
{ // enclosure for using gotos
  if(_function_layer.size() <= 0){
    _tmp_current_fname = "";
    goto skip_to_return;
  }

  _function_data* _data = *(_function_layer.end()-1);
  _tmp_current_fname = _data->name + (_data->is_tailcall? FNAME_TAILCALLED_FLAG: "");
} // enclosure closing

  skip_to_return:{}
  _unlock_object();
}


void execution_flow::_block_execution(){
  _lock_object();
#if (_WIN64) || (_WIN32)
  for(HANDLE hevent: _event_pausing)
    SetEvent(hevent);
#endif
  _currently_executing = false;
  _unlock_object();

  WaitForSingleObject(_execution_block_event, INFINITE);

  _lock_object();
  _currently_executing = true;
#if (_WIN64) || (_WIN32)
  for(HANDLE hevent: _event_resuming)
    SetEvent(hevent);
#endif
  _unlock_object();
}


void execution_flow::_hookcb(){
  _lock_object();
  _currently_executing = true;
  const lua_Debug* _debug_data = _hook_handler->get_current_debug_value();
  _current_line = _debug_data->currentline;

  if(strlen(_debug_data->source) > 0){
    switch(_debug_data->source[0]){
      break; case '@':{
        _current_file_path.clear(); _current_file_path.append(&_debug_data->source[1]);
      }
    }
  }

  switch(_debug_data->event){
    // Debug info does not track function name when it comes to tailcalling

    // update function stack data
    break; case LUA_HOOKTAILCALL:{
      if(_function_layer.size() <= 0)
        break;

      _function_data* _data = *(_function_layer.end()-1);
      _data->is_tailcall = true;

      _update_tmp_current_fname();
    }

    // update function stack data
    break; case LUA_HOOKCALL:{
      _function_data* _new_data = __dm->new_class_dbg<_function_data>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
      _new_data->name = _debug_data->name? _debug_data->name: "???";
      _new_data->is_tailcall = false;

      _function_layer.insert(_function_layer.end(), _new_data);

      _update_tmp_current_fname();
    }

    // update function stack data
    break; case LUA_HOOKRET:{
      if(_function_layer.size() <= 0)
        break;

      auto _iter = _function_layer.end()-1;
      _function_data* _data = *_iter;

      _function_layer.erase(_iter);
      __dm->delete_class_dbg(_data, DYNAMIC_MANAGEMENT_DEBUG_DATA);

      _update_tmp_current_fname();
    }

    // check for breakpoints
    break; case LUA_HOOKLINE:{
      auto _file_iter = _file_metadata_map.find(_current_file_path);
      if(_file_iter != _file_metadata_map.end()){
        _file_metadata* _metadata = _file_iter->second;
        auto _line_iter = _metadata->breakpointed_lines.find(_debug_data->currentline);
        if(_line_iter != _metadata->breakpointed_lines.end())
          goto skip_to_block;
      }
    }
  }

  if(!_do_block)
    goto skip_blocking;
  
  if(_stepping_type != st_none){
    switch(_debug_data->event){
      break; case LUA_HOOKCOUNT:{
        if(_stepping_type != step_type::st_per_counts)
          goto skip_blocking;
      }

      break; case LUA_HOOKLINE:{
        int _layer_count = get_function_layer();
        if(
          _stepping_type != step_type::st_per_line &&
          (_stepping_type != step_type::st_over || _layer_count != _step_layer_check) &&
          _stepping_type != step_type::st_in &&
          (_stepping_type != step_type::st_out || _layer_count > _step_layer_check)
        )
          goto skip_blocking;
      }

      break; case LUA_HOOKTAILCALL:
       case LUA_HOOKCALL:{
        if(_stepping_type != step_type::st_in)
          goto skip_blocking;
      }

      break; case LUA_HOOKRET:{
        int _layer_count = get_function_layer();
        if(
          (_stepping_type != step_type::st_out || _layer_count > _step_layer_check) &&
          (_stepping_type != step_type::st_over || _layer_count > _step_layer_check)
        )
          goto skip_blocking;
      }
    }
  }

  skip_to_block:{}
  
  _unlock_object();
  _block_execution();
  return;

  skip_blocking:{}
  _unlock_object();
  return;
}

void execution_flow::_hookcb_static(const core* lc, void* cb_data){
  ((execution_flow*)cb_data)->_hookcb();
}


void execution_flow::set_step_count(int count){
  _hook_handler->set_count(count);
}

int execution_flow::get_step_count() const{
  return _hook_handler->get_count();
}


unsigned int execution_flow::get_function_layer() const{
  unsigned int _result;
  _lock_object();
  _result = _function_layer.size();
  _unlock_object();
  return _result;
}


const char* execution_flow::get_function_name() const{
  if(_currently_executing)
    return NULL;

  return _tmp_current_fname.c_str();
}


int execution_flow::get_current_line() const{
  if(_currently_executing)  
    return -1;

  return _current_line;
}

const char* execution_flow::get_current_file_path() const{
  if(_currently_executing)
    return NULL;

  return _current_file_path.c_str();
}


bool execution_flow::set_function_name(int layer_idx, const char* name){
  return set_function_name(layer_idx, std::string(name));
}

bool execution_flow::set_function_name(int layer_idx, const std::string& name){
  if(_currently_executing || layer_idx < 0 || layer_idx >= _function_layer.size())
    return false;

  _lock_object();
  _function_data* _data = _function_layer[layer_idx];

  _data->name = name;

  if(layer_idx == (_function_layer.size()-1))
    _update_tmp_current_fname();
  _unlock_object();

  return true;
}


void execution_flow::block_execution(){
  _do_block = true;
  _stepping_type = st_none;
}

void execution_flow::resume_execution(){
  _do_block = false;
#if (_WIN64) || (_WIN32)
  SetEvent(_execution_block_event);
#endif
}

void execution_flow::step_execution(step_type st){
  if(_currently_executing){
    block_execution();
    WaitForSingleObject(_self_pause_event, INFINITE);
  }

  _do_block = true;
  _stepping_type = st;
  switch(st){
    break; case step_type::st_over:{
      _step_layer_check = get_function_layer();
    }

    break; case step_type::st_in:{
      _step_layer_check = get_function_layer()+1;
    }

    break; case step_type::st_out:{
      _step_layer_check = get_function_layer()-1;
    }
  }

  SetEvent(_execution_block_event);
}


bool execution_flow::currently_pausing(){
  return !_currently_executing;
}


void execution_flow::add_breakpoint(const char* file_name, int line){
  _lock_object();

  auto _iter = _file_metadata_map.find(file_name);
  if(_iter == _file_metadata_map.end()){
    _file_metadata_map[file_name] = __dm->new_class_dbg<_file_metadata>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _iter = _file_metadata_map.find(file_name);
  }

  _file_metadata* _metadata = _iter->second;
  _metadata->breakpointed_lines.insert(line);

  _unlock_object();
}

bool execution_flow::remove_breakpoint(const char* file_name, int line){
  bool _result = false;
  _lock_object();

{ // enclosure for using gotos
  auto _iter = _file_metadata_map.find(file_name);
  if(_iter == _file_metadata_map.end())
    goto skip_to_return;

  _file_metadata* _metadata = _iter->second;
  auto _liter = _metadata->breakpointed_lines.find(line);
  if(_liter == _metadata->breakpointed_lines.end())
    goto skip_to_return;

  _metadata->breakpointed_lines.erase(_liter);
  if(_metadata->breakpointed_lines.size() <= 0){
    __dm->delete_class_dbg(_metadata, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _file_metadata_map.erase(_iter);
  }

  _result = true;
} // enclosure closing

  skip_to_return:{}
  _unlock_object();
  return _result;
}

bool execution_flow::clear_breakpoints(const char* file_name){
  bool _result = false;
  _lock_object();

{ // enclosure for using gotos
  auto _iter = _file_metadata_map.find(file_name);
  if(_iter == _file_metadata_map.end())
    goto skip_to_return;

  __dm->delete_class_dbg(_iter->second, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  _file_metadata_map.erase(_iter);

  _result = true;
} // enclosure closing

  skip_to_return:{}
  _unlock_object();
  return _result;
}

void execution_flow::clear_breakpoints(){
  _lock_object();

  for(auto _pair: _file_metadata_map)
    __dm->delete_class_dbg(_pair.second, DYNAMIC_MANAGEMENT_DEBUG_DATA);

  _file_metadata_map.clear();

  _unlock_object();
}


#if (_WIN64) || (_WIN32)
void execution_flow::register_event_resuming(HANDLE hevent){
  _lock_object();
  _event_resuming.insert(hevent);
  _unlock_object();
}

void execution_flow::remove_event_resuming(HANDLE hevent){
  _lock_object();
  _event_resuming.erase(hevent);
  _unlock_object();
}


void execution_flow::register_event_pausing(HANDLE hevent){
  _lock_object();
  _event_pausing.insert(hevent);
  _unlock_object();
}

void execution_flow::remove_event_pausing(HANDLE hevent){
  _lock_object();
  _event_pausing.erase(hevent);
  _unlock_object();
}
#endif


const lua_Debug* execution_flow::get_debug_data() const{
  if(_currently_executing)
    return NULL;
    
  return _hook_handler->get_current_debug_value();
}


void execution_flow::set_logger(I_logger* logger){
  _logger = logger;
}

#endif // LUA_CODE_EXISTS