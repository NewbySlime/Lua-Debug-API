#include "luadebug_executionflow.h"
#include "luainternal_storage.h"
#include "luamemory_util.h"
#include "luastack_iter.h"
#include "luatable_util.h"
#include "luautility.h"
#include "luaruntime_handler.h"
#include "luavariant_util.h"
#include "std_logger.h"


using namespace lua;
using namespace lua::api;
using namespace lua::debug;
using namespace lua::internal;
using namespace lua::memory;
using namespace lua::utility;
using namespace ::memory;


#define FILE_PATH_MAX_BUFFER_SIZE 1024

#define LUD_EXECUTION_FLOW_VARNAME "__clua_execution_flow_data"

#define FNAME_TAILCALLED_FLAG " (tailcalled)"


#ifdef LUA_CODE_EXISTS

static const dynamic_management* __dm = get_memory_manager();


// MARK: lua::debug::execution_flow
execution_flow::execution_flow(lua_State* state){
  _hook_handler = hook_handler::get_this_attached(state);
  _this_state = NULL;
  _logger = get_std_logger();

  if(!state){
    _logger->print_error("Cannot bind execution_flow to lua_State. Reason: lua_State provided is NULL.\n");
    return;
  }

  execution_flow* _check_obj = get_attached_obj(state);
  if(_check_obj){
    _logger->print_warning("Cannot bind execution_flow to lua_State. Reason: Another obj already bound to lua_State.\n");
    return;
  }

  if(!_hook_handler){
    _logger->print_error("Cannot bind execution_flow to lua_State. Reason: Cannot get bound hook_handler in lua_State.\n");
    return;
  }

  _this_state = state;
  _hook_handler->set_hook(LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE | LUA_MASKCOUNT, _hookcb_static, this);

  _set_bind_obj(this, state);
  
  _current_file_path.reserve(FILE_PATH_MAX_BUFFER_SIZE);
}

execution_flow::~execution_flow(){
  if(!_this_state)
    return;

  _hook_handler->remove_hook(this);
  _set_bind_obj(NULL, _this_state);

  while(_function_layer.size() > 0){
    auto _iter = _function_layer.end()-1;
    _function_data* _data = *_iter;
    
    _function_layer.erase(_iter);
    __dm->delete_class(_data);
  }

  if(currently_pausing())
    resume_execution();
}


bool execution_flow::_check_running_thread(){
  runtime_handler* _handler = runtime_handler::get_attached_object(_this_state);

#if (_WIN64) || (_WIN32)
  if(_handler)
    return _handler->get_running_thread_id() != GetCurrentThreadId();
  
  return true;
#endif
}


void execution_flow::_update_tmp_current_fname(){
  if(_function_layer.size() <= 0){
    _tmp_current_fname = "";
    return;
  }

  _function_data* _data = *(_function_layer.end()-1);
  _tmp_current_fname = _data->name + (_data->is_tailcall? FNAME_TAILCALLED_FLAG: "");
}


void execution_flow::_block_execution(){
  std::unique_lock<std::mutex> _ulock(_execution_mutex);

  _currently_executing = false;
#if (_WIN64) || (_WIN32)
  for(HANDLE hevent: _event_pausing)
    SetEvent(hevent);
#endif

  _execution_block.wait(_ulock);

  _currently_executing = true;
#if (_WIN64) || (_WIN32)
  for(HANDLE hevent: _event_resuming)
    SetEvent(hevent);
#endif
}


void execution_flow::_hookcb(){
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
    break; case LUA_HOOKTAILCALL:{
      if(_function_layer.size() <= 0)
        break;

      _function_data* _data = *(_function_layer.end()-1);
      _data->is_tailcall = true;

      _update_tmp_current_fname();
    }

    break; case LUA_HOOKCALL:{
      _function_data* _new_data = __dm->new_class<_function_data>();
      _new_data->name = _debug_data->name? _debug_data->name: "???";
      _new_data->is_tailcall = false;

      _function_layer.insert(_function_layer.end(), _new_data);

      _update_tmp_current_fname();
    }

    break; case LUA_HOOKRET:{
      if(_function_layer.size() <= 0)
        break;

      auto _iter = _function_layer.end()-1;
      _function_data* _data = *_iter;

      _function_layer.erase(_iter);
      __dm->delete_class(_data);

      _update_tmp_current_fname();
    }
  }

  if(!_do_block)
    return;

  if(_stepping_type != st_none){
    switch(_debug_data->event){
      break; case LUA_HOOKCOUNT:{
        if(_stepping_type != step_type::st_per_counts)
          return;
      }

      break; case LUA_HOOKLINE:{
        int _layer_count = get_function_layer();
        if(
          _stepping_type != step_type::st_per_line &&
          (_stepping_type != step_type::st_over || _layer_count != _step_layer_check) &&
          _stepping_type != step_type::st_in &&
          (_stepping_type != step_type::st_out || _layer_count > _step_layer_check)
        )
          return;
      }

      break; case LUA_HOOKTAILCALL:
       case LUA_HOOKCALL:{
        if(_stepping_type != step_type::st_in)
          return;
      }

      break; case LUA_HOOKRET:{
        int _layer_count = get_function_layer();
        if(
          (_stepping_type != step_type::st_out || _layer_count > _step_layer_check) &&
          (_stepping_type != step_type::st_over || _layer_count > _step_layer_check)
        )
          return;
      }
    }
  }

  _block_execution();
}


void execution_flow::_set_bind_obj(execution_flow* obj, lua_State* state){
  require_internal_storage(state); // s+1
  lua_pushstring(state, LUD_EXECUTION_FLOW_VARNAME); // s+1
  lua_gettable(state, -2); // s-1+1
  if(lua_type(state, -1) != LUA_TTABLE){
    lua_pop(state, 1); // s-1
    lua_newtable(state); // s+1

    lua_pushstring(state, LUD_EXECUTION_FLOW_VARNAME); // s+1
    lua_pushvalue(state, -2); // s+1
    lua_settable(state, -4); // s-2
  }

  string_var _key_var = from_pointer_to_str(state);
  lightuser_var _value_var = lightuser_var(obj);

  // possible multiple object in one state for in case of multi threading
  set_table_value(state, -1, &_key_var, &_value_var);

  // pop the global table
  lua_pop(state, 1); // s-1
}


void execution_flow::_hookcb_static(lua_State* state, void* cb_data){
  ((execution_flow*)cb_data)->_hookcb();
}


execution_flow* execution_flow::get_attached_obj(lua_State* state){
  execution_flow* _result = NULL;

  require_internal_storage(state); // s+1
  lua_pushstring(state, LUD_EXECUTION_FLOW_VARNAME); // s+1
  lua_gettable(state, -2); // s-1+1
  if(lua_type(state, -1) != LUA_TTABLE)
    goto skip_checking_label;

  try{
    string_var _key_var = from_pointer_to_str(state);

    variant* _value = get_table_value(state, -1, &_key_var);
    if(_value->get_type() == LUA_TLIGHTUSERDATA)
      _result = (execution_flow*)((lightuser_var*)_value)->get_data();
  }
  catch(std::exception* e){
    printf(e->what());
    __dm->delete_class(e);
  }

  skip_checking_label:{}

  lua_pop(state, 1); // s-1
  return _result;
}


void execution_flow::set_step_count(int count){
  if(!_this_state)
    return;

  _hook_handler->set_count(count);
}

int execution_flow::get_step_count() const{
  if(!_this_state)
    return -1;

  return _hook_handler->get_count();
}


unsigned int execution_flow::get_function_layer() const{
  return _function_layer.size();
}


const char* execution_flow::get_function_name() const{
  return _tmp_current_fname.c_str();
}


int execution_flow::get_current_line() const{
  return _current_line;
}

const char* execution_flow::get_current_file_path() const{
  return _current_file_path.c_str();
}


bool execution_flow::set_function_name(int layer_idx, const char* name){
  return set_function_name(layer_idx, std::string(name));
}

bool execution_flow::set_function_name(int layer_idx, const std::string& name){
  if(layer_idx < 0 || layer_idx >= _function_layer.size())
    return false;

  _function_data* _data = _function_layer[layer_idx];

  _data->name = name;

  if(layer_idx == (_function_layer.size()-1))
    _update_tmp_current_fname();

  return true;
}


// ret_val keep empty if returns void
#define PROHIBIT_SAME_THREAD_EXECUTION_CHECK(ret_val) \
  if(!_check_running_thread()){ \
    if(_logger) \
      _logger->print_error("[execution_flow] Calling function " __FUNCTION__ " using same running thread is prohibited.\n"); \
    return ret_val; \
  }

#define VOID_RET 

void execution_flow::block_execution(){
  if(!_this_state)
    return;
  
  PROHIBIT_SAME_THREAD_EXECUTION_CHECK(VOID_RET)
    
  _do_block = true;
  _stepping_type = st_none;
}

void execution_flow::resume_execution(){
  if(!_this_state)
    return;

  PROHIBIT_SAME_THREAD_EXECUTION_CHECK(VOID_RET)
    
  _do_block = false;
  _execution_block.notify_all();
}

void execution_flow::step_execution(step_type st){
  if(!_this_state)
    return;

  PROHIBIT_SAME_THREAD_EXECUTION_CHECK(VOID_RET)

  block_execution();

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

  _execution_block.notify_all();
}


bool execution_flow::currently_pausing(){
  return !_currently_executing;
}


#if (_WIN64) || (_WIN32)
void execution_flow::register_event_resuming(HANDLE hevent){
  _event_resuming.insert(hevent);
}

void execution_flow::remove_event_resuming(HANDLE hevent){
  _event_resuming.erase(hevent);
}


void execution_flow::register_event_pausing(HANDLE hevent){
  _event_pausing.insert(hevent);
}

void execution_flow::remove_event_pausing(HANDLE hevent){
  _event_pausing.erase(hevent);
}
#endif


const lua_Debug* execution_flow::get_debug_data() const{
  if(!_this_state)
    return NULL;
    
  return _hook_handler->get_current_debug_value();
}


void execution_flow::set_logger(I_logger* logger){
  if(!_this_state)
    return;
    
  _logger = logger;
}


// MARK: DLL definitions

DLLEXPORT lua::debug::I_execution_flow* CPPLUA_CREATE_EXECUTION_FLOW(void* istate){
  return __dm->new_class<execution_flow>((lua_State*)istate);
}

DLLEXPORT void CPPLUA_DELETE_EXECUTION_FLOW(lua::debug::I_execution_flow* object){
  __dm->delete_class(object);
}

#endif // LUA_CODE_EXISTS