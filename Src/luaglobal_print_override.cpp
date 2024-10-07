#include "luainternal_storage.h"
#include "luavariant.h"
#include "luavariant_util.h"
#include "luaglobal_print_override.h"
#include "std_logger.h"
#include "string_util.h"

#define LUD_PRINT_OVERRIDE_VAR_NAME "__clua_print_override"

#if (_WIN64) || (_WIN32)
#define __LOCK_MUTEX(obj) WaitForSingleObject(obj, INFINITE)
#define __RELEASE_MUTEX(obj) ReleaseMutex(obj) 
#endif

using namespace lua;
using namespace lua::global;
using namespace lua::internal;


#ifdef LUA_CODE_EXISTS

// MARK: lua::global::print_override

print_override::print_override(lua_State* state){
  _this_state = NULL;
  _logger = get_std_logger();

#if (_WIN64) || (_WIN32)
  _pipe_read = NULL;
  _pipe_write = NULL;
#endif

  print_override* _current_obj = get_attached_object(state);
  if(_current_obj){
    _logger->print(format_str("Cannot create print_override. Reason: lua_State already has print_override.\n").c_str());
    return;
  }

  _this_state = state;
  _set_bind_obj(this, _this_state);

#if (_WIN64) || (_WIN32)
  CreatePipe(&_pipe_read, &_pipe_write, NULL, 0);

  _class_mutex = CreateMutex(NULL, FALSE, NULL);
#endif

  _bind_global_function();
}

print_override::~print_override(){
  if(!_this_state)
    return;

  _set_bind_obj(NULL, _this_state);
  
#if (_WIN64) || (_WIN32)
  CloseHandle(_pipe_write);
  CloseHandle(_pipe_read);

  CloseHandle(_class_mutex);
#endif
}


void print_override::_bind_global_function(){
  lua_pushglobaltable(_this_state);
  lua_pushcclosure(_this_state, _on_print_static, 0);
  lua_setfield(_this_state, -2, "print");
  lua_pop(_this_state, 1);
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

  __LOCK_MUTEX(_obj->_class_mutex);

#if (_WIN64) || (_WIN32)
  WriteFile(
    _obj->_pipe_write,
    _combined_message.c_str(),
    _combined_message.size(),
    NULL,
    NULL
  );

  for(HANDLE _event_obj: _obj->_event_read)
    SetEvent(_event_obj);
#endif

  __RELEASE_MUTEX(_obj->_class_mutex);

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
  unsigned long _result = 0;

  __LOCK_MUTEX(_class_mutex);

#if (_WIN64) || (_WIN32)
  ReadFile(
    _pipe_read,
    buffer,
    buffer_size,
    &_result,
    NULL
  );
#endif

  __RELEASE_MUTEX(_class_mutex);

  return _result;
}


unsigned long print_override::read_all(I_string_store* store){
  unsigned long _data_len = available_bytes();
  if(_data_len <= 0)
    return 0;

  char* _tmp_buf = (char*)malloc(_data_len);

  __LOCK_MUTEX(_class_mutex);

#if (_WIN64) || (_WIN32)
  unsigned long _read_len;
  ReadFile(
    _pipe_read,
    _tmp_buf,
    _data_len,
    &_read_len,
    NULL
  );
#endif

  __RELEASE_MUTEX(_class_mutex);
  
  store->append(_tmp_buf, _read_len);

  free(_tmp_buf);
  return _read_len;
}

std::string print_override::read_all(){
  unsigned long _data_len = available_bytes();
  char* _tmp_buf = (char*)malloc(_data_len);

  unsigned long _read_len;

  __LOCK_MUTEX(_class_mutex);
  
#if (_WIN64) || (_WIN32)
  ReadFile(
    _pipe_read,
    _tmp_buf,
    _data_len,
    &_read_len,
    NULL
  );
#endif

  __RELEASE_MUTEX(_class_mutex);

  std::string _result = std::string(_tmp_buf, _read_len);

  free(_tmp_buf);
  return _result;
}


unsigned long print_override::peek_n(char* buffer, unsigned long buffer_size){
  unsigned long _result = 0;

  __LOCK_MUTEX(_class_mutex);

#if (_WIN64) || (_WIN32)
  PeekNamedPipe(
    _pipe_read,
    buffer,
    buffer_size,
    &_result,
    NULL,
    NULL
  );
#endif

  __RELEASE_MUTEX(_class_mutex);

  return _result;
}


unsigned long print_override::peek_all(I_string_store* store){
  unsigned long _data_len = available_bytes();
  char* _tmp_buf = (char*)malloc(_data_len);

  unsigned long _read_len;

  __LOCK_MUTEX(_class_mutex);

#if (_WIN64) || (_WIN32)
  PeekNamedPipe(
    _pipe_read,
    _tmp_buf,
    _data_len,
    &_read_len,
    NULL,
    NULL
  );
#endif

  __RELEASE_MUTEX(_class_mutex);

  store->append(_tmp_buf, _read_len);

  free(_tmp_buf);
  return _read_len;
}

std::string print_override::peek_all(){
  unsigned long _data_len = available_bytes();
  char* _tmp_buf = (char*)malloc(_data_len);

  __LOCK_MUTEX(_class_mutex);
  
#if (_WIN64) || (_WIN32)
  unsigned long _read_len;
  PeekNamedPipe(
    _pipe_read,
    _tmp_buf,
    _data_len,
    &_read_len,
    NULL,
    NULL
  );
#endif

  __RELEASE_MUTEX(_class_mutex);
  
  std::string _result = std::string(_tmp_buf, _read_len);

  free(_tmp_buf);
  return _result;
}


unsigned long print_override::available_bytes(){
  unsigned long _result;

#if (_WIN64) || (_WIN32)
  PeekNamedPipe(
    _pipe_read,
    NULL,
    0,
    NULL,
    &_result,
    NULL
  );
#endif

  return _result;
}


#if (_WIN64) || (_WIN32)
void print_override::register_event_read(HANDLE event){
  __LOCK_MUTEX(_class_mutex);
  _event_read.insert(event);
  __RELEASE_MUTEX(_class_mutex);
}

void print_override::remove_event_read(HANDLE event){
  __LOCK_MUTEX(_class_mutex);
  _event_read.erase(event);
  __RELEASE_MUTEX(_class_mutex);
}
#endif


void print_override::set_logger(I_logger* logger){
  __LOCK_MUTEX(_class_mutex);
  _logger = logger;
  __RELEASE_MUTEX(_class_mutex);
}



// MARK: DLL functions

DLLEXPORT lua::global::I_print_override* CPPLUA_CREATE_GLOBAL_PRINT_OVERRIDE(void* state){
  return new print_override((lua_State*)state);
}

DLLEXPORT void CPPLUA_DELETE_GLOBAL_PRINT_OVERRIDE(lua::global::I_print_override* obj){
  delete obj;
}

#endif // LUA_CODE_EXISTS