#include "lua_str.h"
#include "lua_variant.h"
#include "luaglobal_print_override.h"
#include "stdlogger.h"
#include "strutil.h"

#define LUD_PRINT_OVERRIDE_VAR_NAME "__clua_print_override"


using namespace lua;
using namespace lua::global;


// MARK: lua::global::print_override

print_override::print_override(lua_State* state){
  _this_state = NULL;
  _logger = get_stdlogger();

#if (_WIN64) || (_WIN32)
  _pipe_read = NULL;
  _pipe_write = NULL;
#endif

  print_override* _current_obj = get_attached_object(state);
  if(_current_obj){
    _logger->print(format_str("Cannot create print_override. Reason: lua_State already has print_override.\n"));
    return;
  }

  _this_state = state;
  _set_bind_obj(this, _this_state);

#if (_WIN64) || (_WIN32)
  CreatePipe(&_pipe_read, &_pipe_write, NULL, 0);
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
#endif
}


void print_override::_bind_global_function(){
  lua_pushglobaltable(_this_state);
  lua_pushcclosure(_this_state, _on_print_static, 0);
  lua_setfield(_this_state, -2, "print");
  lua_pop(_this_state, 1);
}


void print_override::_set_bind_obj(print_override* obj, lua_State* state){
  lightuser_var _lud_var = obj;
  _lud_var.setglobal(state, LUD_PRINT_OVERRIDE_VAR_NAME);
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

  return 0;
}


print_override* print_override::get_attached_object(lua_State* state){
  variant* _lud_var = to_variant_fglobal(state, LUD_PRINT_OVERRIDE_VAR_NAME);
  print_override* _result =
    (print_override*)(_lud_var->get_type() == lightuser_var::get_static_lua_type()? ((lightuser_var*)_lud_var)->get_data(): NULL);

  delete _lud_var;
  return _result;
}


unsigned long print_override::read_n(char* buffer, unsigned long buffer_size){
  unsigned long _result = 0;

#if (_WIN64) || (_WIN32)
  ReadFile(
    _pipe_read,
    buffer,
    buffer_size,
    &_result,
    NULL
  );
#endif

  return _result;
}


unsigned long print_override::read_all(I_string_store* store){
  unsigned long _data_len = available_bytes();
  char* _tmp_buf = (char*)malloc(_data_len);

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
  
  store->append(_tmp_buf, _read_len);

  free(_tmp_buf);
  return _read_len;
}

std::string print_override::read_all(){
  unsigned long _data_len = available_bytes();
  char* _tmp_buf = (char*)malloc(_data_len);

  unsigned long _read_len;
  
#if (_WIN64) || (_WIN32)
  ReadFile(
    _pipe_read,
    _tmp_buf,
    _data_len,
    &_read_len,
    NULL
  );
#endif

  std::string _result = std::string(_tmp_buf, _read_len);

  free(_tmp_buf);
  return _result;
}


unsigned long print_override::peek_n(char* buffer, unsigned long buffer_size){
  unsigned long _result = 0;

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

  return _result;
}


unsigned long print_override::peek_all(I_string_store* store){
  unsigned long _data_len = available_bytes();
  char* _tmp_buf = (char*)malloc(_data_len);

  unsigned long _read_len;

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

  store->append(_tmp_buf, _read_len);

  free(_tmp_buf);
  return _read_len;
}

std::string print_override::peek_all(){
  unsigned long _data_len = available_bytes();
  char* _tmp_buf = (char*)malloc(_data_len);

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
  _event_read.insert(event);
}

void print_override::remove_event_read(HANDLE event){
  _event_read.erase(event);
}
#endif


void print_override::set_logger(I_logger* logger){
  _logger = logger;
}



// MARK: DLL functions

DLLEXPORT lua::global::I_print_override* CPPLUA_CREATE_GLOBAL_PRINT_OVERRIDE(void* state){
  return new print_override((lua_State*)state);
}

DLLEXPORT void CPPLUA_DELETE_GLOBAL_PRINT_OVERRIDE(lua::global::I_print_override* obj){
  delete obj;
}