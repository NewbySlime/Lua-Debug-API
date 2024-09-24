#include "dllutil.h"
#include "error_util.h"
#include "lua_vararr.h"
#include "lualib_io_handler.h"
#include "luautility.h"
#include "misc.h"
#include "strutil.h"

#include "chrono"


#define TEMPORARY_FILE_EXT ".tmp"

#define FILE_HANDLER_REFERENCE_GLOBAL_DATA "__clua_file_handler_ref_data"
#define FILE_HANDLER_METADATA_OBJECT_POINTER "object"
#define FILE_HANDLER_LUA_OBJECT_TABLE "table_obj"


using namespace dynamic_library::util;
using namespace error::util;
using namespace lua;
using namespace lua::api;
using namespace lua::lib;
using namespace lua::object;
using namespace lua::utility;




// MARK: file_handler definition

static std::map<std::string, file_handler::seek_opt>* _seek_keyword_parser = NULL;
static std::map<std::string, file_handler::buffering_mode>* _buffering_keyword_parser = NULL;

static const fdata _file_handler_fdata[] = {
  fdata("close", file_handler::close),
  fdata("flush", file_handler::flush),
  fdata("lines", file_handler::lines),
  fdata("read", file_handler::read),
  fdata("seek", file_handler::seek),
  fdata("setvbuf", file_handler::setvbuf),
  fdata("write", file_handler::write),
  fdata(NULL, NULL)
};


// MARK: file_handler DLL deinit

void _deinitialize_file_handler_static_vars();
destructor_helper _dh_file_handler(_deinitialize_file_handler_static_vars);

static void _initialize_file_handler_static_vars(){
  if(!_seek_keyword_parser){
    _seek_keyword_parser = new std::map<std::string, file_handler::seek_opt>{
      {"set", file_handler::seek_begin},
      {"cur", file_handler::seek_current},
      {"end", file_handler::seek_end}
    };
  }

  if(!_buffering_keyword_parser){
    _buffering_keyword_parser = new std::map<std::string, file_handler::buffering_mode>{
      {"no", file_handler::buffering_none},
      {"full", file_handler::buffering_full},
      {"line", file_handler::buffering_line}
    };
  }
}

static void _deinitialize_file_handler_static_vars(){
  if(_seek_keyword_parser){
    delete _seek_keyword_parser;
    _seek_keyword_parser = NULL;
  }

  if(_buffering_keyword_parser){
    delete _buffering_keyword_parser;
    _buffering_keyword_parser = NULL;
  }
}


static void _def_fhandler_destructor(I_object* obj){
  delete obj;
}


file_handler::file_handler(void* istate, const compilation_context* context, const std::string& path, int op): function_store(_def_fhandler_destructor){
  _initialize_file_handler_static_vars();

  _lua_state = istate;
  _current_context = context;

  set_function_data(_file_handler_fdata);
  
  // create copy of path in case of temporary file 
  std::string _actual_path = path;

#if (_WIN64) || (_WIN32)
  DWORD _access_type = 0;
  DWORD _open_type = 0;
  DWORD _flags = 0;

  // op code checks
  if(op & open_temporary)
    op = open_write | open_special;

  if(op & open_append)
    op = op | open_preserve;

  // adjust io behaviour
  if(op & open_special)
    _access_type = GENERIC_ALL;
  else
    _access_type = (op & open_write)? GENERIC_WRITE: GENERIC_READ;

  // adjust opening behaviour
  _open_type = (op & open_write)? OPEN_ALWAYS: OPEN_EXISTING;

  // if temporary, ignore path argument, create a new file with time as its name
  if(op & open_temporary){
    _actual_path = create_random_name() + TEMPORARY_FILE_EXT;
    _flags |= FILE_ATTRIBUTE_TEMPORARY;
  }

  HANDLE _file = CreateFileA(
    _actual_path.c_str(),
    _access_type,
    0,
    NULL,
    _open_type,
    _flags,
    NULL
  );

  // check if error
  if(_file == INVALID_HANDLE_VALUE){
    _update_last_error_winapi();
    return;
  }

  // check if preserve
  if(!(op & open_preserve)){
    SetFilePointer(_file, 0, NULL, FILE_BEGIN);
    SetEndOfFile(_file);
  }

  // check if append
  if(op & open_append)
    _minimal_write_offset = GetFileSize(_file, NULL);

  _op_code = op;
  _hfile = _file;
#endif

  _object_metadata = _create_metadata(istate, context, this);
}

#if (_WIN64) || (_WIN32)
file_handler::file_handler(void* istate, const compilation_context* context, HANDLE pipe_handle, bool is_output): function_store(_def_fhandler_destructor){
  _initialize_file_handler_static_vars();

  _lua_state = istate;
  _current_context = context;

  set_function_data(_file_handler_fdata);

  _hfile = pipe_handle;
  _is_pipe_handle = true;
  _own_handle = false;

  _op_code = is_output? open_write: open_read;

  _object_metadata = _create_metadata(istate, context, this);
}
#endif


file_handler::~file_handler(){
  _delete_object_table();
  _delete_object_metadata();

  flush();
  close();
  _clear_error();
}


void file_handler::_clear_error(){
  if(!_last_error)
    return;

  delete _last_error;
  _last_error = NULL;
}

void file_handler::_set_last_error(long long err_code, const std::string& err_msg){
  _clear_error();

  string_var _str = err_msg.c_str();
  _last_error = new error_var(&_str, err_code);
}

#if (_WIN64) || (_WIN32)
void file_handler::_update_last_error_winapi(){
  DWORD _errcode = GetLastError();
  std::string _errmsg = get_windows_error_message(_errcode);
  _set_last_error(_errcode, _errmsg);
}
#endif


void file_handler::_delete_object_metadata(){
  _delete_metadata(_lua_state, _current_context, this);
  if(_object_metadata){
    delete _object_metadata;
    _object_metadata = NULL;
  }
}

void file_handler::_delete_object_table(){
  if(_object_metadata){
    string_var _keystr = FILE_HANDLER_LUA_OBJECT_TABLE;
    nil_var _nilv;
    _object_metadata->set_value((I_variant*)&_keystr, (I_variant*)&_nilv); // directly casting to not make the compiler upset
  }

  if(_object_table){
    delete _object_table;
    _object_table = NULL;
  }
}


void file_handler::_skip_str(bool(*filter_cb)(char ch)){
  if(already_closed()){
    _set_last_error(ERROR_HANDLES_CLOSED, "File already closed.");
    return;
  }

  if(!can_read()){
    _set_last_error(-1, "Read operation is prohibited.");
    return;
  }

  char _tmpc;
  while(!end_of_file_reached()){
    // don't use _seek, pipe does not have seek feature
    // _seek(seek_current, 1);

#if (_WIN64) || (_WIN32)
    bool _success = PeekNamedPipe(_hfile, &_tmpc, sizeof(char), NULL, NULL, NULL);
    if(!_success)
      goto on_windows_error;

    if(!filter_cb(_tmpc))
      break;

    _success = ReadFile(_hfile, &_tmpc, sizeof(char), NULL, NULL);
    if(!_success)
      goto on_windows_error;
#endif
  }

  return;


  // ERROR GOTOs

#if (_WIN64) || (_WIN32)
  on_windows_error:{
    _update_last_error_winapi();
  }
#endif
}

int file_handler::_read_str(char* buffer, size_t buflen, bool(*filter_cb)(char ch)){
  if(end_of_file_reached())
    return -1;

  if(already_closed()){
    _set_last_error(ERROR_HANDLES_CLOSED, "File already closed.");
    return;
  }

  if(!can_read()){
    _set_last_error(-1, "Read operation is prohibited.");
    return -1;
  }

  size_t _cur_idx = 0;
  char _tmpc;
  while(!end_of_file_reached() && _cur_idx < buflen){
    // don't use _seek, pipe does not have seek feature
    // _seek(seek_current, 1);

#if (_WIN64) || (_WIN32)
    bool _success = PeekNamedPipe(_hfile, &_tmpc, sizeof(char), NULL, NULL, NULL);
    if(!_success)
      goto on_windows_error;

    if(filter_cb && filter_cb(_tmpc))
      break;

    _success = ReadFile(_hfile, &_tmpc, sizeof(char), NULL, NULL);
    if(!_success)
      goto on_windows_error;
#endif

    buffer[_cur_idx] = _tmpc;
    _cur_idx++;
  }

  buffer[_cur_idx] = '\0';
  return _cur_idx;


  // ERROR GOTOs

#if (_WIN64) || (_WIN32)
  on_windows_error:{
    _update_last_error_winapi();

    buffer[_cur_idx] = '\0';
    return _cur_idx;
  }
#endif
}


void file_handler::_check_write_pos(){
  long _current_pos = seek(seek_current, 0);
  if(_current_pos < _minimal_write_offset)
    seek(seek_begin, _minimal_write_offset);
}


void file_handler::_fill_error_with_last(I_vararr* res){
  nil_var _nil;
  number_var _num = _last_error->get_error_code();

  res->append_var(&_nil);
  res->append_var(_last_error);
  res->append_var(&_num);
}


void file_handler::_require_global_table(void* istate, const compilation_context* c){
  int _type = c->api_value->getglobal(istate, FILE_HANDLER_REFERENCE_GLOBAL_DATA);
  if(_type == LUA_TTABLE)
    return;

  c->api_value->newtable(istate);
  c->api_stack->pushvalue(istate, -1); // create copy for the result
  c->api_value->setglobal(istate, FILE_HANDLER_REFERENCE_GLOBAL_DATA);
}

table_var* file_handler::_create_metadata(void* istate, const compilation_context* c, file_handler* obj){
  return pstack_call<table_var*>(istate, 0, 0, c->api_stack, c->api_value, [](void* istate, const compilation_context* c, file_handler* obj){
    _require_global_table(istate, c);

    // create metadata table
    std::string _pkey = _get_pointer_str(obj);
    c->api_value->pushstring(istate, _pkey.c_str());
    c->api_value->newtable(istate);
    table_var* _table_ref = new table_var(istate, -1, c);

    // set object pointer
    c->api_value->pushstring(istate, FILE_HANDLER_METADATA_OBJECT_POINTER);
    c->api_value->pushlightuserdata(istate, obj);
    c->api_value->settable(istate, -3);

    // set metadata table
    c->api_value->settable(istate, -3);

    // stack stabilizing handled by pstack_call()
    return _table_ref;
  }, istate, c, obj);
}

void file_handler::_delete_metadata(void* istate, const compilation_context* c, file_handler* obj){
  // since reference table is just set nil in the global table, pointer in the reference table should also set to nil
  
  pstack_call<void>(istate, 0, 0, c->api_stack, c->api_value, [](void* istate, const compilation_context* c, file_handler* obj){
    _require_global_table(istate, c);

    // get metadata table
    std::string _pkey = _get_pointer_str(obj);
    c->api_value->pushstring(istate, _pkey.c_str());
    c->api_stack->pushvalue(istate, -1); // create key copy for checking
    if(c->api_value->gettable(istate, -3) != LUA_TTABLE)
      return; // stack stabilizing handled by pstack_call()

    // set pointer to NULL
    c->api_value->pushstring(istate, FILE_HANDLER_METADATA_OBJECT_POINTER);
    c->api_value->pushlightuserdata(istate, NULL);
    c->api_value->settable(istate, -3);

    // remove the reference table from stack
    c->api_stack->pop(istate, 1);

    // set metadata table to nil
    c->api_value->pushnil(istate);
    c->api_value->settable(istate, -3);
  }, istate, c, obj);
}


std::string file_handler::_get_pointer_str(const void* pointer){
  return format_str("0x%X", pointer);
}


file_handler* file_handler::_get_file_handler(void* istate, const compilation_context* c){
  return pstack_call<file_handler*>(istate, 0, 0, c->api_stack, c->api_value, [](void* istate, const compilation_context* c, file_handler* obj){
    if(c->api_value->type(istate, -1) != LUA_TTABLE)
      return (file_handler*)NULL;

    c->api_value->pushstring(istate, FILE_HANDLER_METADATA_OBJECT_POINTER);
    c->api_value->gettable(istate, -2);

    if(c->api_value->type(istate, -1) != LUA_TLIGHTUSERDATA)
      return (file_handler*)NULL;

    return (file_handler*)c->api_value->touserdata(istate, -1);
  }, istate, c);
}


int file_handler::_lua_line_cb(void* istate, const compilation_context* c){
  file_handler* _this;
  int _arg_count = 0;
  int _ret_count = 0;

  error_var* _err = pstack_call_context<error_var*>(istate, 0, 0, c, [](void* istate, const compilation_context* c, file_handler** _pthis, int* _parg_count){
    c->api_stack->pushvalue(istate, lua_upvalueindex(1)); // get metadata
    if(c->api_value->type(istate, -1) != LUA_TTABLE){
      string_var _errmsg = "Function is not properly setup; Object table is nil.";
      return new error_var(&_errmsg, -1);
    }

    c->api_value->pushstring(istate, FILE_HANDLER_METADATA_OBJECT_POINTER);
    c->api_value->gettable(istate, -2);
    if(c->api_value->type(istate, -1) != LUA_TLIGHTUSERDATA){
      string_var _errmsg = "Object already deleted.";
      return new error_var(&_errmsg, -1);
    }

    (*_pthis) = (file_handler*)c->api_value->touserdata(istate, -1);
    c->api_stack->pop(istate, 2); // pop metadata table and object pointer

    c->api_stack->pushvalue(istate, lua_upvalueindex(2)); // get arg count
    if(c->api_value->type(istate, -1) != LUA_TNUMBER){
      string_var _errmsg = "Function is not properly setup; Arg count variable is not an int.";
      return new error_var(&_errmsg, -1);
    }

    (*_parg_count) = c->api_value->tointeger(istate, -1);
    c->api_stack->pop(istate, 1); // pop arg count
  }, istate, c, &_this, &_arg_count);

  if(_err){
    c->api_varutil->push_variant(istate, _err->get_error_data());
    delete _err;

    c->api_util->error(istate);
  }


  vararr _args;
  vararr _results;

  for(int i = 1; i <= _arg_count; i++){
    I_variant* _arg = c->api_varutil->to_variant(istate, lua_upvalueindex(i+2));
    _args.append_var(_arg);
    c->api_varutil->delete_variant(_arg);
  }

  read(_this, &_args, &_results);

  // NOTE: error throwing is not possible in read

  for(int i = 0; i < _results.get_var_count(); i++)
    c->api_varutil->push_variant(istate, _results.get_var(i));

  _ret_count = _results.get_var_count();

  return _ret_count;
}


const I_error_var* file_handler::get_last_error() const{
  return _last_error;
}


void file_handler::close(I_object* obj, const I_vararr* args, I_vararr* res){
  file_handler* _this = dynamic_cast<file_handler*>(obj);
  _this->_clear_error();

  _this->close();

  if(_this->get_last_error())
    _this->_fill_error_with_last(res);
}

void file_handler::flush(I_object* obj, const I_vararr* args, I_vararr* res){
  file_handler* _this = dynamic_cast<file_handler*>(obj);
  _this->_clear_error();

  _this->flush();

  if(_this->get_last_error())
    _this->_fill_error_with_last(res);
}

void file_handler::lines(I_object* obj, const I_vararr* args, I_vararr* res){
  // don't store I_object pointer directly, if possible, use reference object like tables
  file_handler* _this = dynamic_cast<file_handler*>(obj);
  if(_this->already_closed()){
    _this->_set_last_error(ERROR_HANDLES_CLOSED, "File already closed.");
    res->append_var(_this->get_last_error());

    return;
  }

  // iterator function
  function_var _iterfunc(_lua_line_cb);
  
  // set metadata table as first upvalue
  _iterfunc.get_arg_closure()->append_var(_this->_object_metadata);
  _this->_current_context->api_stack->pop(_this->_lua_state, 1);

  // set how many values to read
  number_var _count = args->get_var_count();
  _iterfunc.get_arg_closure()->append_var(&_count);

  // push this parameter to iterator function's upvalues
  _iterfunc.get_arg_closure()->append(args);

  // add iterator function as result
  res->append_var(&_iterfunc);
}

void file_handler::read(I_object* obj, const I_vararr* args, I_vararr* res){
  // n: read number on the first word
  // a: read all data
  // l: read a line, without the newline character
  // L: read a line, with the newline character
  // (number): read a number of bytes
  file_handler* _this = dynamic_cast<file_handler*>(obj);

  _this->_clear_error();

  for(int i = 0; i < args->get_var_count(); i++){
    if(_this->end_of_file_reached())
      goto on_error_eof;

    // skip reading if error already present from the previous argument
    if(_this->get_last_error())
      goto on_error_eof;

    const I_variant* _iarg = args->get_var(i); 
    if(_iarg->get_type() == LUA_TSTRING){
      const I_string_var* _str_var = dynamic_cast<const I_string_var*>(_iarg);
      const char* _cstr = _str_var->get_string();
      if(_cstr[0] == '*') // lua53, skip the wildcard character
        _cstr = &_cstr[1];
      
      switch(_cstr[0]){
        break; case 'n':{ // MARK: READ 'n'
          bool (*_filter_cb)(char) = [](char c){return c == ' ';};
          bool (*_filter_cb_invert)(char) = [](char c){return c != ' ';};

          char* _buffer = (char*)malloc(IO_MAXIMUM_ALL_READ_LEN+1); // with null termination

          _this->_skip_str(_filter_cb); // (if available) skip the first whitespaces
          int _read_len = _this->_read_str(_buffer, IO_MAXIMUM_ALL_READ_LEN, _filter_cb);
          _this->_skip_str(_filter_cb_invert); // (if available) skip unread characters until whitespace
          _this->_skip_str(_filter_cb); // skip next whitespace

          number_var _num = atof(_buffer);
          res->append_var(&_num);

          free(_buffer);
        }

        break; case 'a':{ // MARK: READ 'a'
          char* _buffer = (char*)malloc(IO_MAXIMUM_ALL_READ_LEN+1); // with null termination
          int _read_len = _this->_read_str(_buffer, IO_MAXIMUM_ALL_READ_LEN);

          string_var _str(_buffer, _read_len);
          res->append_var(&_str);

          free(_buffer);
        }

        break; case 'L':
        break; case 'l':{ // MARK: READ 'l' or 'L'
          bool (*_filter_cb)(char) = [](char c){return c == '\n' || c == '\r';};

          char* _buffer = (char*)malloc(IO_MAXIMUM_ALL_READ_LEN+2); // with null termination also newline character (if possible)
          
          int _read_len = _this->_read_str(_buffer, IO_MAXIMUM_ALL_READ_LEN, _filter_cb);

          char _next_char = _this->peek(); 
          if(_cstr[0] == 'L' && _filter_cb(_next_char)){
            _buffer[IO_MAXIMUM_ALL_READ_LEN] = _next_char;
            _buffer[IO_MAXIMUM_ALL_READ_LEN+1] = '\0';

            _read_len++;
          }
          
          _this->_skip_str(_filter_cb);

          string_var _str(_buffer, _read_len);
          res->append_var(&_str);

          free(_buffer);
        }

        break; default:
          goto on_error_format;
      }
    }
    else if(_iarg->get_type() == LUA_TNUMBER){ // MARK: READ based on a number
      const I_number_var* _number_var = dynamic_cast<const I_number_var*>(_iarg);
      char* _buffer = (char*)malloc(IO_MAXIMUM_ALL_READ_LEN+1);

      size_t _read_len = (size_t)_number_var->get_number();
      if(_number_var->get_number() > 0)
        _read_len = _this->_read_str(_buffer, _read_len);
      else
        _read_len = 0;

      string_var _str(_buffer, _read_len);
      res->append_var(&_str);

      free(_buffer);
    }
    else
      goto on_error_format;

    // check for errors (will be skipped if current arguments are after the first)
    const I_error_var* _errvar = _this->get_last_error();
    if(i <= 0 && _errvar)
      goto on_error;

    continue;
    on_error_eof:{
      nil_var _nvar;
      res->append_var(&_nvar);
    }
  }

  return;


  // ERROR GOTOs

  on_error_format:{
    _this->_set_last_error(ERROR_BAD_FORMAT, "Invalid format in file:read function.");
    goto on_error;
  }

  on_error:{
    res->clear();
    _this->_fill_error_with_last(res);
  }
}

void file_handler::seek(I_object* obj, const I_vararr* args, I_vararr* res){
  file_handler* _this = dynamic_cast<file_handler*>(obj);

  if(_this->already_closed()){
    _this->_set_last_error(ERROR_HANDLES_CLOSED, "File already closed.");
    goto on_error;
  }
  
{// enclosure for using goto

  // Getting arguments
  seek_opt _seekmode = seek_current;
  {const I_variant* _check_var;
    if(args->get_var_count() < 1)
      goto skip_argument_parsing_arg1;

    _check_var = args->get_var(0);
    if(_check_var->get_type() != I_string_var::get_static_lua_type()){
      _this->_set_last_error(ERROR_BAD_ARGUMENTS, "Argument #1: Not a type of string.");
      goto on_error;
    }

    const I_string_var* _strmode = dynamic_cast<const I_string_var*>(_check_var);
    auto _iter = _seek_keyword_parser->find(_strmode->get_string());
    if(_iter == _seek_keyword_parser->end()){
      _this->_set_last_error(ERROR_BAD_ARGUMENTS, "Argument #1: Invalid mode.");
      goto on_error;
    }

    _seekmode = _iter->second;
  }skip_argument_parsing_arg1:{}

  long _offset = 0;
  {const I_variant* _check_var;
    if(args->get_var_count() < 2)
      goto skip_argument_parsing_arg2;

    _check_var = args->get_var(1);
    if(_check_var->get_type() != I_number_var::get_static_lua_type()){
      _this->_set_last_error(ERROR_BAD_ARGUMENTS, "Argument #2: Not a type of number.");
      goto on_error;
    }

    const I_number_var* _offset_var = dynamic_cast<const I_number_var*>(_check_var);
    _offset = _offset_var->get_number();
  }skip_argument_parsing_arg2:{}


  // Running the code
  _this->_clear_error();
  long _result = _this->seek(_seekmode, _offset);
  if(_this->get_last_error())
    goto on_error;


  // Add result for current seek position
  number_var _res_var = _result;
  res->append_var(&_res_var);

}// enclosure closing

  return;


  // ERROR GOTOs

  on_error:{
    res->clear();
    _this->_fill_error_with_last(res);
  }
}

void file_handler::setvbuf(I_object* obj, const I_vararr* args, I_vararr* res){
  file_handler* _this = dynamic_cast<file_handler*>(obj);

  if(_this->already_closed()){
    _this->_set_last_error(ERROR_HANDLES_CLOSED, "File already closed.");
    goto on_error;
  }

{// enclosure for using gotos

  // Getting arguments
  if(args->get_var_count() < 1){
    _this->_set_last_error(ERROR_BAD_ARGUMENTS, "Not enough arguments.");
    goto on_error;
  }

  buffering_mode _mode;
  {const I_variant* _check_var;
    _check_var = args->get_var(0);
    if(_check_var->get_type() != I_string_var::get_static_lua_type()){
      _this->_set_last_error(ERROR_BAD_ARGUMENTS, "Argument #1: Not a type of string.");
      goto on_error;
    }

    const I_string_var* _strmode = dynamic_cast<const I_string_var*>(_check_var);
    auto _iter = _buffering_keyword_parser->find(_strmode->get_string());
    if(_iter == _buffering_keyword_parser->end()){
      _this->_set_last_error(ERROR_BAD_ARGUMENTS, "Argument #1: Invalid mode.");
      goto on_error;
    }

    _mode = _iter->second;
  }

  // Second argument: buffering size are handled internally by Windows

  _this->set_buffering_mode(_mode);

}// enclosure closing

  return;


  // ERROR GOTOs

  on_error:{
    res->clear();
    _this->_fill_error_with_last(res);
  }
}

void file_handler::write(I_object* obj, const I_vararr* args, I_vararr* res){
  file_handler* _this = dynamic_cast<file_handler*>(obj);
  
{// enclosure for using gotos
  _this->_clear_error();
  for(int i = 0; i < args->get_var_count(); i++){
    const I_variant* _check_var = args->get_var(i);

    string_store _arg_str;
    switch(_check_var->get_type()){
      break; case I_number_var::get_static_lua_type():
      break; case I_string_var::get_static_lua_type():
        _check_var->to_string(&_arg_str);

      break; default:{
        _this->_set_last_error(ERROR_BAD_ARGUMENTS, "Invalid argument.");
        goto on_error;
      }
    }
  
    _this->write(_arg_str.data.c_str(), _arg_str.data.length());
    if(_this->get_last_error())
      goto on_error;
  }

}// enclosure closing

  return;


  // ERROR GOTOs
  
  on_error:{
    res->clear();
    _this->_fill_error_with_last(res);
  }
}


bool file_handler::close(){
#if (_WIN64) || (_WIN32)
  if(!_hfile){
    _set_last_error(ERROR_HANDLES_CLOSED, "File already closed.");
    return false;
  }

  _delete_object_table();

  if(_own_handle){
    flush();
    CloseHandle(_hfile);
  }

  _hfile = NULL;
#endif

  return true;
}

bool file_handler::flush(){
#if (_WIN64) || (_WIN32)
  if(!_hfile){ 
    _set_last_error(ERROR_HANDLES_CLOSED, "File already closed.");
    return false;
  }

  FlushFileBuffers(_hfile);
#endif

  return true;
}

char file_handler::peek(){
  char _tmpc;

  if(end_of_file_reached())
    return '\0';

#if (_WIN64) || (_WIN32)
  bool _success = PeekNamedPipe(_hfile, &_tmpc, sizeof(char), NULL, NULL, NULL);
  if(!_success)
    goto on_windows_error;
#endif

  return _tmpc;


  // ERROR GOTOs

#if (_WIN64) || (_WIN32)
  on_windows_error:{
    _update_last_error_winapi();
    return '\0';
  }
#endif
}

long file_handler::seek(seek_opt opt, long offset){
  long _result = -1;

  if(_is_pipe_handle)
    return _result;

#if (_WIN64) || (_WIN32)
  DWORD _filesize = GetFileSize(_hfile, NULL);
  if(_filesize == INVALID_FILE_SIZE)
    goto on_windows_error;

  DWORD _currentpos = SetFilePointer(_hfile, 0, NULL, FILE_CURRENT);
  if(_currentpos == INVALID_FILE_SIZE)
    goto on_windows_error;

  // NOTE: WINAPI does not support negative values as the new seek value
  // using loop as seek_begin used as a pivot
  bool _keep_loop = true;
  while(_keep_loop){
    switch(opt){
      break; case seek_begin:{
        if(offset < 0)
          offset = 0;
        else if(offset > _filesize)
          offset = _filesize;

        _keep_loop = false;
      }

      break; case seek_current:{
        offset += _currentpos;
        opt = seek_begin;
      }

      break; case seek_end:{
        offset += _filesize;
        opt = seek_begin;
      }
    }
  }

  _result = SetFilePointer(_hfile, offset, NULL, FILE_BEGIN);
  if(_result == INVALID_FILE_SIZE)
    goto on_windows_error;

#endif

  return _result;


  // ERROR GOTOs

#if (_WIN64) || (_WIN32)
  on_windows_error:{
    _update_last_error_winapi();
    return -1;
  }
#endif
}

size_t file_handler::read(char* buffer, size_t buffer_size){
  return _read_str(buffer,  buffer_size);
}

size_t file_handler::write(const char* buffer, size_t buffer_size){
  size_t _result = 0;

  if(!can_write()){
    _set_last_error(-1, "Write operation is prohibited.");
    return 0;
  }

  // check appending mode
  _check_write_pos();

#if (_WIN64) || (_WIN32)
  DWORD _written_len = 0;
  bool _success = WriteFile(
    _hfile,
    buffer,
    buffer_size,
    &_written_len,
    NULL
  );

  if(!_success)
    goto on_windows_error;

  _result = _written_len;
#endif

  switch(_buffering_mode){
    break; case buffering_none:{
      flush();
    }

    break; case buffering_line:{
      character_filter _filter = [](char c){ return c == '\n' || c == '\r'; };
      if(string_find_character(buffer, buffer_size, _filter) != SIZE_MAX)
        flush();
    }

    // buffering_full: let the OS handle it
  }

  return _result;


  // ERROR GOTOs
#if (_WIN64) || (_WIN32)
  on_windows_error:{
    _update_last_error_winapi();
    return 0;
  }
#endif
}

void file_handler::set_buffering_mode(buffering_mode mode){
  _buffering_mode = mode;
}


bool file_handler::already_closed() const{
  return !_hfile;  
}

bool file_handler::end_of_file_reached() const{
  return get_remaining_read() <= 0;
}

size_t file_handler::get_remaining_read() const{
  size_t _result = 0;

#if (_WIN64) || (_WIN32)
  DWORD _available_bytes = 0;
  PeekNamedPipe(_hfile, NULL, 0, NULL, &_available_bytes, NULL);
  _result = _available_bytes;
#endif

  return _result;
}


bool file_handler::can_write() const{
  return (_op_code & open_write) > 0 || (_op_code & open_special) > 0;
}

bool file_handler::can_read() const{
  return (_op_code & open_read) > 0 || (_op_code & open_special) > 0;
}


I_debuggable_object* file_handler::as_debug_object(){
  return this;
}

void file_handler::on_object_added(void* istate, const lua::api::compilation_context* c){
  _delete_object_table();

  string_var _keystr = FILE_HANDLER_LUA_OBJECT_TABLE;
  _object_table = new table_var(istate, -1, c);
  _object_metadata->set_value((I_variant*)&_keystr, (I_variant*)_object_table); // directly casting to not make the compiler upset
}