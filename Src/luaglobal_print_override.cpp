#include "luaglobal_print_override.h"
#include "luainternal_storage.h"
#include "luamemory_util.h"
#include "luavariant.h"
#include "luavariant_util.h"
#include "memory.h"
#include "std_logger.h"
#include "string_util.h"

#include "defines.h"


#define LUD_PRINT_OVERRIDE_VAR_NAME "__clua_print_override"

#define BUFFER_SIZE 0x10000


using namespace lua;
using namespace lua::global;
using namespace lua::internal;
using namespace lua::memory;
using namespace ::memory;


#ifdef LUA_CODE_EXISTS

static const I_dynamic_management* __dm = get_memory_manager();


// MARK: lua::global::print_override
print_override::print_override(lua_State* state){
  _this_state = NULL;
  _logger = get_std_logger();

  _class_mutex_ptr = &_class_mutex;

  print_override* _current_obj = get_attached_object(state);
  if(_current_obj){
    _logger->print("Cannot create print_override. Reason: lua_State already has print_override.\n");
    return;
  }

  _this_state = state;
  _set_bind_obj(this, _this_state);

  _buffer_data = (char*)malloc(BUFFER_SIZE);
  _buffer_len = BUFFER_SIZE;
  if(!_buffer_data)
    _logger->print_warning("Cannot create buffer.\n");

  _bind_global_function();
}

print_override::~print_override(){
  if(!_this_state)
    return;

  _set_bind_obj(NULL, _this_state);
  
  if(_buffer_data){
    free(_buffer_data);
    _buffer_data = NULL;
  }
}


void print_override::_bind_global_function(){
  lua_pushglobaltable(_this_state);
  lua_pushcclosure(_this_state, _on_print_static, 0);
  lua_setfield(_this_state, -2, "print");
  lua_pop(_this_state, 1);
}


size_t print_override::_remaining_read_buffer(){
  if(!_buffer_data)
    return 0;

  if(!_buffer_filled)
    return 0;

  if(_start_iteration_index >= _iteration_index)
    return _iteration_index + (_buffer_len-_start_iteration_index);

  return _iteration_index-_start_iteration_index;
}

size_t print_override::_read_buffer(char* dest, size_t dest_len, bool is_peeking){
  if(!_buffer_data)
    return 0;

  size_t _previous_iteration_idx = _start_iteration_index;
  bool _previous_buffer_filled = _buffer_filled;

  if(!_buffer_filled)
    return 0;

  size_t _read_idx = 0;
  while(_read_idx < dest_len){
    size_t _buffer_remaining = _buffer_len-_start_iteration_index;
    if(_start_iteration_index < _iteration_index)
      _buffer_remaining = _iteration_index-_start_iteration_index;
    
    size_t _read_remaining = dest_len-_read_idx;

    size_t _read_len = std::min(_buffer_remaining, _read_remaining);
    memcpy(&dest[_read_idx], &_buffer_data[_start_iteration_index], _read_len);

    if(_start_iteration_index < _iteration_index && _start_iteration_index+_read_len >= _iteration_index)
      _buffer_filled = false;

    _read_idx += _read_len;
    _start_iteration_index += _read_len;

    // reset the index
    _start_iteration_index = _start_iteration_index%_buffer_len;
  }

  if(is_peeking){
    _start_iteration_index = _previous_iteration_idx;
    _buffer_filled = _previous_buffer_filled;
  }

  return _read_idx;
}

size_t print_override::_write_buffer(const char* src, size_t src_len){
  if(!_buffer_data)
    return 0;

  size_t _write_idx = 0;
  while(_write_idx < src_len){
    size_t _buffer_remaining = _buffer_len-_iteration_index;
    size_t _write_remaining = src_len-_write_idx;

    size_t _write_len = std::min(_buffer_remaining, _write_remaining);
    memcpy(&_buffer_data[_iteration_index], &src[_write_idx], _write_len);

    if(_buffer_filled && _start_iteration_index == _iteration_index)
      _start_iteration_index += _write_idx;

    _write_idx += _write_len;
    _iteration_index += _write_len;

    // reset the index
    _iteration_index = _iteration_index%_buffer_len;
    _buffer_filled = true;
  }

  return _write_idx;
}


void print_override::_lock_object() const{
  _class_mutex_ptr->lock();
}

void print_override::_unlock_object() const{
  _class_mutex_ptr->unlock();
}


void print_override::_set_bind_obj(print_override* obj, lua_State* state){
  require_internal_storage(state); // s+1

  lua_pushstring(state, LUD_PRINT_OVERRIDE_VAR_NAME); // s+1
  lua_pushlightuserdata(state, obj); // s+1
  lua_settable(state, -3); // s-2

  // pop internal storage
  lua_pop(state, 1); // s+1
}

int print_override::_on_print_static(lua_State* state){
  print_override* _obj = get_attached_object(state);
  if(!_obj || !_obj->_this_state)
    return 0;

  std::string _combined_message;

  int _n_args = lua_gettop(state);
  for(int i = 0; i < _n_args; i++){
    std::string _message = to_string(state, i+1);
    _combined_message += _message;
    
    if(i < (_n_args+1))
      _combined_message += "\t";
  }

  _combined_message += "\n";

  _obj->_lock_object();

  _obj->_write_buffer(_combined_message.c_str(), _combined_message.size());  
  
#if (_WIN64) || (_WIN32)
  for(HANDLE _event_obj: _obj->_event_read)
    SetEvent(_event_obj);
#elif (__linux)
  for(int _event_obj: _obj->_event_read)
    eventfd_write(_event_obj, 1);
#endif

  _obj->_unlock_object();

  return 0;
}


print_override* print_override::get_attached_object(lua_State* state){
  print_override* _result = NULL;
  require_internal_storage(state); // s+1

  lua_pushstring(state, LUD_PRINT_OVERRIDE_VAR_NAME); // s+1
  lua_gettable(state, -2); // s-1+1
  if(lua_type(state, -1) == LUA_TLIGHTUSERDATA)
    _result = (print_override*)lua_touserdata(state, -1);

  // pop internal storage and gettable result
  lua_pop(state, 2); // s-2
  return _result;
}


unsigned long print_override::read_n(char* buffer, unsigned long buffer_size){
  _lock_object();
  unsigned long _result = _read_buffer(buffer, buffer_size);
  _unlock_object();

  return _result;
}


unsigned long print_override::read_all(I_string_store* store){
  unsigned long _data_len = available_bytes();
  if(_data_len <= 0)
    return 0;

  char* _tmp_buf = (char*)__dm->malloc(_data_len, DYNAMIC_MANAGEMENT_DEBUG_DATA);

  _lock_object();
  unsigned long _read_len = _read_buffer(_tmp_buf, _data_len);
  _unlock_object();
  
  store->append(_tmp_buf, _read_len);

  __dm->free(_tmp_buf, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  return _read_len;
}

std::string print_override::read_all(){
  unsigned long _data_len = available_bytes();
  char* _tmp_buf = (char*)__dm->malloc(_data_len, DYNAMIC_MANAGEMENT_DEBUG_DATA);

  _lock_object();
  unsigned long _read_len = _read_buffer(_tmp_buf, _data_len);
  _unlock_object();

  std::string _result = std::string(_tmp_buf, _read_len);

  __dm->free(_tmp_buf, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  return _result;
}


unsigned long print_override::peek_n(char* buffer, unsigned long buffer_size){
  _lock_object();
  unsigned long _result = _read_buffer(buffer, buffer_size, true);
  _unlock_object();

  return _result;
}


unsigned long print_override::peek_all(I_string_store* store){
  unsigned long _data_len = available_bytes();
  char* _tmp_buf = (char*)__dm->malloc(_data_len, DYNAMIC_MANAGEMENT_DEBUG_DATA);

  _lock_object();
  unsigned long _read_len = _read_buffer(_tmp_buf, _data_len, true);
  _unlock_object();

  store->append(_tmp_buf, _read_len);

  __dm->free(_tmp_buf, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  return _read_len;
}

std::string print_override::peek_all(){
  unsigned long _data_len = available_bytes();
  char* _tmp_buf = (char*)__dm->malloc(_data_len, DYNAMIC_MANAGEMENT_DEBUG_DATA);

  _lock_object();
  unsigned long _read_len = _read_buffer(_tmp_buf, _data_len, true);
  _unlock_object();
  
  std::string _result = std::string(_tmp_buf, _read_len);

  __dm->free(_tmp_buf, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  return _result;
}


unsigned long print_override::available_bytes(){
  return _remaining_read_buffer();
}


#if (_WIN64) || (_WIN32)

void print_override::register_event_read(HANDLE event){
  _lock_object();
  _event_read.insert(event);
  _unlock_object();
}

void print_override::remove_event_read(HANDLE event){
  _lock_object();
  _event_read.erase(event);
  _unlock_object();
}

#elif (__linux)

void print_override::register_event_read(int event_object){
  _lock_object();
  _event_read.insert(event_object);
  _unlock_object();
}

void print_override::remove_event_read(int event_object){
  _lock_object();
  _event_read.erase(event_object);
  _unlock_object();
}

#endif


void* print_override::get_lua_interface_state() const{
  return _this_state;
}


void print_override::set_logger(I_logger* logger){
  _lock_object();
  _logger = logger;
  _unlock_object();
}



// MARK: DLL functions

DLLEXPORT lua::global::I_print_override* CPPLUA_CREATE_GLOBAL_PRINT_OVERRIDE(void* state){
  return __dm->new_class_dbg<print_override>(DYNAMIC_MANAGEMENT_DEBUG_DATA, (lua_State*)state);
}

DLLEXPORT void CPPLUA_DELETE_GLOBAL_PRINT_OVERRIDE(lua::global::I_print_override* obj){
  __dm->delete_class_dbg(obj, DYNAMIC_MANAGEMENT_DEBUG_DATA);
}

#endif // LUA_CODE_EXISTS