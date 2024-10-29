#include "dllutil.h"
#include "error_util.h"
#include "luaapi_util.h"
#include "luaapi_variant_util.h"
#include "lualibrary_iohandler.h"
#include "luamemory_util.h"
#include "luaobject_util.h"
#include "luatable_util.h"
#include "luautility.h"
#include "luavariant.h"
#include "luavariant_arr.h"
#include "luavariant_util.h"
#include "misc.h"
#include "string_util.h"

#include "chrono"
#include "memory"
#include "set"

#if (_WIN64) || (_WIN32)
#include "Windows.h"
#include "winerror.h"
#endif


#define IO_HANDLER_STDOUT_VARNAME "stdout"
#define IO_HANDLER_STDIN_VARNAME "stdin"
#define IO_HANDLER_STDERR_VARNAME "stderr"

#define TEMPORARY_FILE_EXT ".tmp"

#define FILE_HANDLER_REFERENCE_INTERNAL_DATA "__clua_file_handler_ref_data"
#define FILE_HANDLER_METADATA_OBJECT_POINTER "object"
#define FILE_HANDLER_LUA_OBJECT_TABLE "table_obj"

#define FILE_HANDLER_LINES_DEFAULT_VALUE1 "*l"


using namespace dynamic_library::util;
using namespace error::util;
using namespace lua;
using namespace lua::api;
using namespace lua::library;
using namespace lua::memory;
using namespace lua::object;
using namespace lua::utility;
using namespace ::memory;


#ifdef LUA_CODE_EXISTS

static I_dynamic_management* __dm = get_memory_manager();


// MARK: Library DLL variables

static std::map<std::string, file_handler::seek_opt>* _seek_keyword_parser = NULL;
static std::map<std::string, file_handler::buffering_mode>* _buffering_keyword_parser = NULL;

static void _deinitialize_file_handler_static_vars();
destructor_helper _dh_file_handler(_deinitialize_file_handler_static_vars);

static void _initialize_file_handler_static_vars(){
  if(!_seek_keyword_parser){
    _seek_keyword_parser = __dm->new_class_dbg<std::map<std::string, file_handler::seek_opt>>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
      _seek_keyword_parser->insert({"set", file_handler::seek_begin});
      _seek_keyword_parser->insert({"cur", file_handler::seek_current});
      _seek_keyword_parser->insert({"end", file_handler::seek_end});
  }

  if(!_buffering_keyword_parser){
    _buffering_keyword_parser = __dm->new_class_dbg<std::map<std::string, file_handler::buffering_mode>>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
      _buffering_keyword_parser->insert({"no", file_handler::buffering_none});
      _buffering_keyword_parser->insert({"full", file_handler::buffering_full});
      _buffering_keyword_parser->insert({"line", file_handler::buffering_line});
  }
}

static void _deinitialize_file_handler_static_vars(){
  if(_seek_keyword_parser){
    __dm->delete_class_dbg(_seek_keyword_parser, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _seek_keyword_parser = NULL;
  }

  if(_buffering_keyword_parser){
    __dm->delete_class_dbg(_buffering_keyword_parser, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _buffering_keyword_parser = NULL;
  }
}




// MARK: io_handler definition

static const fdata _io_handler_fdata[] = {
  fdata("close", io_handler::close),
  fdata("flush", io_handler::flush),
  fdata("open", io_handler::open),
  fdata("input", io_handler::input),
  fdata("output", io_handler::output),
  fdata("error", io_handler::error),
  fdata("lines", io_handler::lines),
  fdata("read", io_handler::read),
  fdata("tmpfile", io_handler::tmpfile),
  fdata("type", io_handler::type),
  fdata("write", io_handler::write),
  fdata(NULL, NULL) // array closing
};


#define IO_HANDLER_REPLACE_DEF_FILE(def_file) \
  io_handler* _this = dynamic_cast<io_handler*>(obj); \
  _this->lock_object(); \
  _this->_clear_error(); \
   \
{ /* enclosure for using goto */ \
   \
  /* Getting arguments */ \
  {const I_variant* _check_var; \
    if(args->get_var_count() < 1){ /* push default input file when argument is empty */ \
      res->append_var(_this->def_file); \
      return; \
    } \
     \
    _check_var = args->get_var(0); \
    switch(_check_var->get_type()){ \
      /* set default input file from table data */ \
      break; case I_table_var::get_static_lua_type():{ \
        const I_table_var* _itvar = dynamic_cast<const I_table_var*>(_check_var); \
        if(!file_handler::check_object_validity(_itvar)){ \
          _this->_set_last_error(ERROR_BAD_ARGUMENTS, "Argument #1: File handle object is invalid."); \
          goto on_error; \
        } \
         \
        _itvar->push_to_stack(&_this->_lc); \
        _this->def_file->from_state(&_this->_lc, -1); \
        _this->_lc.context->api_stack->pop(&_this->_lc, 1); \
      } \
       \
      /* set default input file from file name */ \
      break; case I_string_var::get_static_lua_type():{ \
        const I_string_var* _strvar = dynamic_cast<const I_string_var*>(_check_var); \
        file_handler* _tmp_obj = __dm->new_class_dbg<file_handler>(DYNAMIC_MANAGEMENT_DEBUG_DATA, &_this->_lc, _strvar->get_string(), file_handler::open_read); \
        if(_tmp_obj->get_last_error()){ \
          _this->_copy_error_from(_tmp_obj); \
          __dm->delete_class_dbg(_tmp_obj, DYNAMIC_MANAGEMENT_DEBUG_DATA); \
           \
          goto on_error; \
        } \
         \
        object_var _tmp_var(&_this->_lc, _tmp_obj); \
        _tmp_var.push_to_stack(&_this->_lc); \
        _this->def_file->from_state(&_this->_lc, -1); \
        _this->_lc.context->api_stack->pop(&_this->_lc, 1); \
      } \
       \
      break; default:{ \
        _this->_set_last_error(ERROR_BAD_ARGUMENTS, "Argument #1: Not a file handle."); \
        goto on_error; \
      } \
    } \
  } \
   \
} /* enclosure closing */ \
   \
  _this->unlock_object(); \
  return; \
   \
   \
  /* ERROR GOTOs */ \
   \
  on_error:{ \
    res->clear(); \
    res->append_var(_this->get_last_error()); \
    _this->unlock_object(); \
  } \

#define IO_HANDLER_REDIRECT_FUNCTION(table_obj, fname) \
  io_handler* _this = dynamic_cast<io_handler*>(obj); \
  _this->lock_object(); \
  _this->_clear_error(); \
   \
  string_var _fname = fname; \
  const I_function_var* _fvar = dynamic_cast<const I_function_var*>(table_obj->get_value(_fname)); \
  _fvar->run_function(&_this->_lc, args, res); \
  _this->_copy_error_from(res); \
  /* delete result from table_var */ \
  _this->_filein_def->get_lua_core()->context->api_varutil->delete_variant(_fvar); \
  _this->unlock_object(); \
  
#define IO_HANDLER_SET_FILE(file, target_table) \
  bool _result = true; \
  lock_object(); \
{ /* enclosure for using gotos */ \
  _clear_error(); \
   \
  if(!file){ \
    _set_last_error(ERROR_INVALID_HANDLE, "file_handler is NULL."); \
    _result = false; \
    goto skip_to_return; \
  } \
   \
  object_var _tmpobj(&_lc, file); \
  _tmpobj.push_to_stack(&_lc); \
  target_table->from_state(&_lc, -1); \
  _lc.context->api_stack->pop(&_lc, 1); \
} /* enclosure closing */ \
  skip_to_return:{} \
  unlock_object(); \
  return _result; \



static void _def_iohandler_destructor(I_object* obj){
  __dm->delete_class_dbg(obj, DYNAMIC_MANAGEMENT_DEBUG_DATA);
}

io_handler::io_handler(const lua::api::core* lc, const constructor_param* param): function_store(_def_iohandler_destructor){
  set_function_data(_io_handler_fdata);

#if (_WIN64) || (_WIN32)
  _object_mutex_ptr = &_object_mutex;
  InitializeCriticalSection(_object_mutex_ptr);
#endif

  I_file_handler* _stdout = NULL;
  I_file_handler* _stdin = NULL;
  I_file_handler* _stderr = NULL;
  if(param){
    _stdout = param->stdout_file;
    _stdin = param->stdin_file;
    _stderr = param->stderr_file;
  }

#if (_WIN64) || (_WIN32)
  if(!_stdout){
    _stdout = __dm->new_class_dbg<file_handler>(DYNAMIC_MANAGEMENT_DEBUG_DATA, lc, GetStdHandle(STD_OUTPUT_HANDLE), true);
  }

  if(!_stdin){
    _stdin = __dm->new_class_dbg<file_handler>(DYNAMIC_MANAGEMENT_DEBUG_DATA, lc, GetStdHandle(STD_INPUT_HANDLE), false);
  }

  if(!_stderr){
    _stderr = __dm->new_class_dbg<file_handler>(DYNAMIC_MANAGEMENT_DEBUG_DATA, lc, GetStdHandle(STD_ERROR_HANDLE), true);
  }
#endif

  _stdout_object = __dm->new_class_dbg<object_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, lc, _stdout);
  _stdin_object = __dm->new_class_dbg<object_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, lc, _stdin);
  _stderr_object = __dm->new_class_dbg<object_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, lc, _stderr);

  _stdout_object->push_to_stack(lc);
  _fileout_def = __dm->new_class_dbg<table_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, lc, -1);
  lc->context->api_stack->pop(lc->istate, 1);

  _stdin_object->push_to_stack(lc);
  _filein_def = __dm->new_class_dbg<table_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, lc, -1);
  lc->context->api_stack->pop(lc->istate, 1);

  _stderr_object->push_to_stack(lc);
  _fileerr_def = __dm->new_class_dbg<table_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, lc, -1);
  lc->context->api_stack->pop(lc->istate, 1);

  _lc = *lc;
}

io_handler::~io_handler(){
  if(_obj_data){
    __dm->delete_class_dbg(_obj_data, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _obj_data = NULL;
  }

  __dm->delete_class_dbg(_stdout_object, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  __dm->delete_class_dbg(_stdin_object, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  __dm->delete_class_dbg(_stderr_object, DYNAMIC_MANAGEMENT_DEBUG_DATA);

  flush();
  close();

  __dm->delete_class_dbg(_fileout_def, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  __dm->delete_class_dbg(_filein_def, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  __dm->delete_class_dbg(_fileerr_def, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  
  _clear_error();

#if (_WIN64) || (_WIN32)
  DeleteCriticalSection(_object_mutex_ptr);
#endif
}


void io_handler::_clear_error(){
  if(_last_error){
    __dm->delete_class_dbg(_last_error, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _last_error = NULL;
  }
}

void io_handler::_set_last_error(long long err_code, const std::string& err_msg){
  _clear_error();
  string_var _str = err_msg;
  _last_error = __dm->new_class_dbg<error_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, &_str, err_code);
}

void io_handler::_copy_error_from(file_handler* file){
  _clear_error();
  _last_error = __dm->new_class_dbg<error_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, file->get_last_error());
}

void io_handler::_copy_error_from(const I_vararr* data){
  if(!_has_error(data))
    return;

  _clear_error();
  string_store _errmsg;
  data->get_var(1)->to_string(&_errmsg);
  _set_last_error(-1, _errmsg.data);
}

bool io_handler::_has_error(const I_vararr* data){
  if(data->get_var_count() < 2)
    return false;

  return data->get_var(0)->get_type() == I_nil_var::get_static_lua_type();
}

#if (_WIN64) || (_WIN32)
void io_handler::_update_last_error_winapi(){
  DWORD _errcode = GetLastError();
  std::string _errmsg = get_windows_error_message(_errcode);
  _set_last_error(_errcode, _errmsg);
}
#endif


void io_handler::_fill_error_with_last(I_vararr* res){
  nil_var _nil;
  number_var _num = _last_error->get_error_code();

  res->append_var(&_nil);
  res->append_var(_last_error);
  res->append_var(&_num);
}


const I_error_var* io_handler::get_last_error() const{
  return _last_error;
}


void io_handler::close(I_object* obj, const I_vararr* args, I_vararr* res){
  io_handler* _this = dynamic_cast<io_handler*>(obj);
  _this->lock_object();
  _this->_clear_error();

{ // enclosure for using goto

  {const I_variant* _check_var;
    if(args->get_var_count() >= 1){ 
      _check_var = args->get_var(0);
      switch(_check_var->get_type()){
        break; case I_table_var::get_static_lua_type():{
          const I_table_var* _itvar = dynamic_cast<const I_table_var*>(_check_var);
          if(!file_handler::check_object_validity(_itvar)){
            _this->_set_last_error(ERROR_BAD_ARGUMENTS, "Argument #1: Not a file handle; invalid object.");
            goto on_error;
          }

          string_var _strkey = "close";
          // no way that it could fail after checking its validity
          const I_function_var* _fvar = dynamic_cast<const I_function_var*>(_itvar->get_value(&_strkey));

          vararr _args;
          vararr _res;

          _fvar->run_function(_itvar->get_lua_core(), &_args, &_res);

          res->clear();
          res->append(&_res);

          // delete result from table_var
          _itvar->get_lua_core()->context->api_varutil->delete_variant(_fvar);

          if(_this->_has_error(res))
            _this->_copy_error_from(res);
        }

        break; default:{
          _this->_set_last_error(ERROR_BAD_ARGUMENTS, "Argument #1: Not a file handle; wrong type.");
        }
      }
    }
    else // closes default file if argument is empty
      _this->close();
  }

  if(_this->get_last_error())
    goto on_error;

}

  _this->unlock_object();
  return;

  on_error:{
    res->clear();
    _this->_fill_error_with_last(res);
    _this->unlock_object();
  }
}

void io_handler::flush(I_object* obj, const I_vararr* args, I_vararr* res){
  io_handler* _this = dynamic_cast<io_handler*>(obj);
  _this->lock_object();
  _this->_clear_error();

  _this->flush();

  if(_this->get_last_error()){
    res->clear();
    _this->_fill_error_with_last(res);
  }

  _this->unlock_object();
}

void io_handler::open(I_object* obj, const I_vararr* args, I_vararr* res){
  io_handler* _this = dynamic_cast<io_handler*>(obj);
  _this->lock_object();
  _this->_clear_error();

{ // enclosure for using goto

  std::string _fname_str;
  std::string _mode_str = "r";

  // Getting arguments
  {const I_variant* _check_var;
    if(args->get_var_count() < 1){
      _this->_set_last_error(ERROR_BAD_ARGUMENTS, "Not enough argument.");
      goto on_error;
    }

    // get file name
    _check_var = args->get_var(0);
    if(_check_var->get_type() != I_string_var::get_static_lua_type()){
      _this->_set_last_error(ERROR_BAD_ARGUMENTS, "Argument #1: Not a string.");
      goto on_error;
    }

    _fname_str = dynamic_cast<const I_string_var*>(_check_var)->get_string();
    
    // get mode
    if(args->get_var_count() < 2)
      goto skip_argument_parse;

    _check_var = args->get_var(1);
    if(_check_var->get_type() != I_string_var::get_static_lua_type()){
      _this->_set_last_error(ERROR_BAD_ARGUMENTS, "Argument #2: Not a string.");
      goto on_error;
    }

    _mode_str = dynamic_cast<const I_string_var*>(_check_var)->get_string();
  }

  skip_argument_parse:{}

  const int _filter_io = ~(file_handler::open_read | file_handler::open_write | file_handler::open_append);
  int _open_mode = file_handler::open_read;
  for(int i = 0; i < _mode_str.length(); i++){
    char _c = _mode_str[i];
    switch(_c){
      break; case 'r':
        _open_mode &= _filter_io;
        _open_mode |= file_handler::open_read;

      break; case 'w':
        _open_mode &= _filter_io;
        _open_mode |= file_handler::open_write;

      break; case 'a':
        _open_mode &= _filter_io;
        _open_mode |= file_handler::open_append;

      break; case '+':
        _open_mode |= file_handler::open_special;

      break; case 'b':
        _open_mode |= file_handler::open_binary;
    }
  }

  file_handler* _file = __dm->new_class_dbg<file_handler>(DYNAMIC_MANAGEMENT_DEBUG_DATA, &_this->_lc, _fname_str, _open_mode);
  if(_file->get_last_error()){
    _this->_copy_error_from(_file);
    __dm->delete_class_dbg(_file, DYNAMIC_MANAGEMENT_DEBUG_DATA);

    goto on_error;
  }

  object_var _tmpobj(&_this->_lc, _file);
  res->append_var(&_tmpobj);

} // enclosure closing

  _this->unlock_object();
  return;


  on_error:{
    res->clear();
    _this->_fill_error_with_last(res);
    _this->unlock_object();
  }
}

void io_handler::input(I_object* obj, const I_vararr* args, I_vararr* res){
  IO_HANDLER_REPLACE_DEF_FILE(_filein_def)
} 

void io_handler::output(I_object* obj, const I_vararr* args, I_vararr* res){
  IO_HANDLER_REPLACE_DEF_FILE(_fileout_def)
}

void io_handler::error(I_object* obj, const I_vararr* args, I_vararr* res){
  IO_HANDLER_REPLACE_DEF_FILE(_fileerr_def)
}

void io_handler::lines(I_object* obj, const I_vararr* args, I_vararr* res){
  io_handler* _this = dynamic_cast<io_handler*>(obj);
  _this->lock_object();
  _this->_clear_error();

{ // enclosure for using goto
  table_var _target_obj;
  vararr _farg;

  // Getting arguments
  {const I_variant* _check_var;
    if(args->get_var_count() < 1){ // push iterator function from default input
      _target_obj = *_this->_filein_def;
      goto skip_parse_argument;
    }

    _check_var = args->get_var(0);
    if(_check_var->get_type() == I_string_var::get_static_lua_type()){
      const I_string_var* _file_name = dynamic_cast<const I_string_var*>(_check_var);

      // let it leak, let the Lua GC handle it
      file_handler* _file = __dm->new_class_dbg<file_handler>(DYNAMIC_MANAGEMENT_DEBUG_DATA, &_this->_lc, _file_name->get_string(), file_handler::open_read | file_handler::open_automatic_close);
      if(_file->get_last_error()){
        _this->_copy_error_from(_file);
        __dm->delete_class_dbg(_file, DYNAMIC_MANAGEMENT_DEBUG_DATA);

        goto on_error;
      }

      object_var _tmp_obj(&_this->_lc, _file);
      _tmp_obj.push_to_stack(&_this->_lc);
      _target_obj.from_state(&_this->_lc, -1);
      _this->_lc.context->api_stack->pop(_this->_lc.istate, 1);
    }
    else{
      _this->_set_last_error(ERROR_BAD_ARGUMENTS, "Argument #1: Not a file name.");
      goto on_error;
    }

    for(int i = 1; i < args->get_var_count(); i++)
      _farg.append_var(args->get_var(i));
  }

  skip_parse_argument:{}

  string_var _fname = "lines";
  // no way that the function could fail
  const I_function_var* _func = dynamic_cast<const I_function_var*>(_target_obj.get_value(_fname));
  res->clear();

  int _errcode = _func->run_function(_target_obj.get_lua_core(), &_farg, res);
  
  // delete result from table_var
  _target_obj.get_lua_core()->context->api_varutil->delete_variant(_func);

  if(_errcode != LUA_OK){
    string_store _errmsg;
    if(res->get_var_count() >= 1)
      res->get_var(0)->to_string(&_errmsg);

    _this->_set_last_error(_errcode, _errmsg.data);
  }

} // enclosure closing

  _this->unlock_object();
  return;


  on_error:{
    res->clear();
    res->append_var(_this->get_last_error());
    _this->unlock_object();
  }
}

void io_handler::read(I_object* obj, const I_vararr* args, I_vararr* res){
  IO_HANDLER_REDIRECT_FUNCTION(_this->_filein_def, "read")
}

void io_handler::tmpfile(I_object* obj, const I_vararr* args, I_vararr* res){
  io_handler* _this = dynamic_cast<io_handler*>(obj);
  _this->lock_object();
  _this->_clear_error();

{ // enclosure for using goto
  file_handler* _file = __dm->new_class_dbg<file_handler>(DYNAMIC_MANAGEMENT_DEBUG_DATA, &_this->_lc, std::string(), file_handler::open_temporary);
  if(_file->get_last_error()){
    _this->_copy_error_from(_file);
    __dm->delete_class_dbg(_file, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    goto on_error;
  }

  object_var _tmpobj(&_this->_lc, _file);
  res->append_var(&_tmpobj);
} // enclosure closing

  _this->unlock_object();
  return;


  on_error:{
    res->clear();
    _this->_fill_error_with_last(res);
    _this->unlock_object();
  }
}

void io_handler::type(I_object* obj, const I_vararr* args, I_vararr* res){
  io_handler* _this = dynamic_cast<io_handler*>(obj);
  _this->lock_object();
  _this->_clear_error();

{ // enclosure for using goto
  if(args->get_var_count() < 1)
    goto on_invalid;

  const I_variant* _check_var = args->get_var(0);
  if(_check_var->get_type() != I_table_var::get_static_lua_type())
    goto on_invalid;

  const I_table_var* _tvar = dynamic_cast<const I_table_var*>(_check_var);
  if(!file_handler::check_object_validity(_tvar))
    goto on_invalid;

  {
    const string_var _case_closed = "closed file";
    const string_var _case_normal = "file";

    string_var _fname = "read";
    const I_function_var* _fvar = dynamic_cast<const I_function_var*>(_tvar->get_value(&_fname));

    vararr _farg;{
      number_var _num = 0.f; _farg.append_var(&_num);
    }
    vararr _fres;

    _fvar->run_function(&_this->_lc, &_farg, &_fres);
    if(_this->_has_error(&_fres))
      res->append_var(&_case_closed);
    else
      res->append_var(&_case_normal);
  
    _tvar->get_lua_core()->context->api_varutil->delete_variant(_fvar);
  }

} // enclosure closing

  _this->unlock_object();
  return;

  on_invalid:{
    res->clear();
    nil_var _nvar;
    res->append_var(&_nvar);
    _this->unlock_object();
  }
}

void io_handler::write(I_object* obj, const I_vararr* args, I_vararr* res){
  IO_HANDLER_REDIRECT_FUNCTION(_this->_fileout_def, "write")
}


void io_handler::close(){
  lock_object();
  _clear_error();

  bool _skip_output_closing = false;
{ // enclosure for using goto
  _fileout_def->push_to_stack(&_lc);
  if(_lc.context->api_varutil->get_special_type(_lc.istate, -1) != I_object_var::get_static_lua_type())
    goto skip_output_parsing;

  object_var _out_obj(&_lc, -1);
  if(_out_obj.get_object_reference() == _stdout_object->get_object_reference())
    _skip_output_closing = true;
} // enclosure closing
  
  skip_output_parsing:{}
  _lc.context->api_stack->pop(_lc.istate, 1);

  if(!_skip_output_closing){
    string_var _fname = "close";
    const I_function_var* _fvar = dynamic_cast<const I_function_var*>(_fileout_def->get_value(_fname));

    vararr _fargs;
    vararr _fres;

    _fvar->run_function(&_lc, &_fargs, &_fres);

    _fileout_def->get_lua_core()->context->api_varutil->delete_variant(_fvar);

    if(_has_error(&_fres)) // check if returns an error data
      _copy_error_from(&_fres);
  }

  unlock_object();
}

void io_handler::flush(){
  lock_object();
  _clear_error();

  string_var _fname = "flush";
  vararr _fargs;
  vararr _fres;
  
  // Run flush to output
  const I_function_var* _fvar = dynamic_cast<const I_function_var*>(_fileout_def->get_value(_fname));
  _fvar->run_function(&_lc, &_fargs, &_fres);
  _fileout_def->get_lua_core()->context->api_varutil->delete_variant(_fvar);

  if(_has_error(&_fres)) // check if returns an error data
    _copy_error_from(&_fres);

  _fres.clear();
  
  // Run flush to error
  _fvar = dynamic_cast<const I_function_var*>(_fileerr_def->get_value(_fname));
  _fvar->run_function(&_lc, &_fargs, &_fres);
  _fileerr_def->get_lua_core()->context->api_varutil->delete_variant(_fvar);
  
  if(_has_error(&_fres)) // check if returns an error data
    _copy_error_from(&_fres);

  unlock_object();
}


bool io_handler::set_input_file(I_file_handler* file){
  IO_HANDLER_SET_FILE(file, _filein_def)
}

bool io_handler::set_output_file(I_file_handler* file){
  IO_HANDLER_SET_FILE(file, _fileout_def)
}

bool io_handler::set_error_file(I_file_handler* file){
  IO_HANDLER_SET_FILE(file, _fileerr_def)
}


const I_table_var* io_handler::get_input_file() const{
  return _filein_def;
}

const I_table_var* io_handler::get_output_file() const{
  return _fileout_def;
}

const I_table_var* io_handler::get_error_file() const{
  return _fileerr_def;
}


size_t io_handler::read(char* buffer, size_t buffer_size){
  lock_object();
  _clear_error();

  string_var _fname = "read";
  const I_function_var* _fvar = dynamic_cast<const I_function_var*>(_filein_def->get_value(_fname));
  vararr _fargs;
  vararr _fres;

  size_t _read_idx = 0;
  while(_read_idx < buffer_size){
    {_fargs.clear();
      number_var _read_len = buffer_size - _read_idx; _fargs.append_var(&_read_len); 
    }
    _fres.clear();

    _fvar->run_function(&_lc, &_fargs, &_fres);

    if(_fres.get_var_count() < 1) // abnormalities detected
      break;

    const I_variant* _check_var = _fres.get_var(0);
    if(_check_var->get_type() != I_string_var::get_static_lua_type()) // abnormalities detected
      break;

    const I_string_var* _res_str = dynamic_cast<const I_string_var*>(_check_var);
    memcpy(&buffer[_read_idx], _res_str->get_string(), _res_str->get_length());
    _read_idx += _res_str->get_length();
  }

  if(_has_error(&_fres)) // check if the function returns an error data
    _copy_error_from(&_fres);

  _filein_def->get_lua_core()->context->api_varutil->delete_variant(_fvar);

  unlock_object();
  return _read_idx;
}

size_t io_handler::read(I_string_store* pstr, size_t read_len){
  lock_object();
  _clear_error();

  char* _tmpbuf = (char*)__dm->malloc(read_len, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  size_t _read_len = read(_tmpbuf, read_len);
  pstr->append(_tmpbuf, _read_len);
  __dm->free(_tmpbuf, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  
  unlock_object();
  return _read_len;
}

size_t io_handler::write(const char* buffer, size_t buffer_size){
  lock_object();
  _clear_error();

  string_var _fname_write = "write";
  string_var _fname_seek = "seek";
  const I_function_var* _fvar_write = dynamic_cast<const I_function_var*>(_fileout_def->get_value(_fname_write));
  const I_function_var* _fvar_seek = dynamic_cast<const I_function_var*>(_fileout_def->get_value(_fname_seek));

  const I_variant* _check_var;

  size_t _fp_begin = 0;
  size_t _fp_end = 0;

  vararr _fargs_seek;{
    string_var _seek_mode = "set"; _fargs_seek.append_var(&_seek_mode);
  }

  vararr _fargs;
  vararr _fres;

  // Seek for getting current file position

  _fvar_seek->run_function(&_lc, &_fargs_seek, &_fres);
  if(_fres.get_var_count() < 1) // abnormalities detected
    goto skip_to_return;
  
  _check_var = _fres.get_var(0);
  if(_check_var->get_type() != I_number_var::get_static_lua_type()) // abnormalities detected
    goto skip_to_return;

  _fp_begin = dynamic_cast<const I_number_var*>(_check_var)->get_number();
  _fp_end = _fp_begin;

  // Write the data

  _fargs.clear();{
    string_var _buffer_copy(buffer, buffer_size); _fargs.append_var(&_buffer_copy);
  }
  _fres.clear();

  _fvar_write->run_function(&_lc, &_fargs, &_fres);

  if(_fres.get_var_count() < 1) // abnormalities detected
    goto skip_to_return;

  if(_fres.get_var(0)->get_type() == I_nil_var::get_static_lua_type()) // error
    goto skip_to_return;

  // Seek for getting new file position

  _fres.clear();
  _fvar_seek->run_function(&_lc, &_fargs_seek, &_fres);
  _check_var = _fres.get_var(0); // always success after previous check to seek function
  if(_check_var->get_type() != I_number_var::get_static_lua_type())
    goto skip_to_return;
  _fp_end = dynamic_cast<const I_number_var*>(_check_var)->get_number();

  skip_to_return:{}

  if(_has_error(&_fres)) // check if previous function returns an error data
    _copy_error_from(&_fres);
  
  _fileout_def->get_lua_core()->context->api_varutil->delete_variant(_fvar_write);
  _fileout_def->get_lua_core()->context->api_varutil->delete_variant(_fvar_seek);

  unlock_object();
  return _fp_end - _fp_begin;
}


I_debuggable_object* io_handler::as_debug_object(){
  return NULL;
}

void io_handler::on_object_added(const core* lc){
  lock_object();
  _obj_data = __dm->new_class_dbg<table_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, lc, -1);
  
  string_var _str = IO_HANDLER_STDOUT_VARNAME;
  _obj_data->set_value((I_variant*)&_str, (I_variant*)_stdout_object); // directly casting to not make the compiler upset

  _str = IO_HANDLER_STDIN_VARNAME;
  _obj_data->set_value((I_variant*)&_str, (I_variant*)_stdin_object);

  _str = IO_HANDLER_STDERR_VARNAME;
  _obj_data->set_value((I_variant*)&_str, (I_variant*)_stderr_object);

  // Reset the lua_core to make sure to use this compilation context.
  _lc = *lc;
  unlock_object();
}


void io_handler::lock_object() const{
#if (_WIN64) || (_WIN32)
  EnterCriticalSection(_object_mutex_ptr);
#endif
}

void io_handler::unlock_object() const{
#if (_WIN64) || (_WIN32)
  LeaveCriticalSection(_object_mutex_ptr);
#endif
}




// MARK: file_handler definition

static const fdata _file_handler_fdata[] = {
  fdata("close", file_handler::close),
  fdata("flush", file_handler::flush),
  fdata("lines", file_handler::lines),
  fdata("read", file_handler::read),
  fdata("seek", file_handler::seek),
  fdata("setvbuf", file_handler::setvbuf),
  fdata("write", file_handler::write),
  fdata(NULL, NULL) // array closing
};


static void _def_fhandler_destructor(I_object* obj){
  __dm->delete_class_dbg(obj, DYNAMIC_MANAGEMENT_DEBUG_DATA);
}


file_handler::file_handler(const core* lc, const std::string& path, int op): function_store(_def_fhandler_destructor){
  _initialize_file_handler_static_vars();
  set_function_data(_file_handler_fdata);

#if (_WIN64) || (_WIN32)
  _object_mutex_ptr = &_object_mutex;
  InitializeCriticalSection(_object_mutex_ptr);
#endif

  _lc = *lc;
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
  _is_pipe_handle = false;
  _own_handle = true;
#endif

  _initiate_reference();
}

#if (_WIN64) || (_WIN32)
file_handler::file_handler(const core* lc, HANDLE pipe_handle, bool is_output): function_store(_def_fhandler_destructor){
  _initialize_file_handler_static_vars();
  set_function_data(_file_handler_fdata);

#if (_WIN64) || (_WIN32)
  _object_mutex_ptr = &_object_mutex;
  InitializeCriticalSection(_object_mutex_ptr);
#endif

  _lc = *lc;
  _hfile = pipe_handle;
  _is_pipe_handle = true;
  _own_handle = false;

  _op_code = is_output? open_write: open_read;

  _initiate_reference();
}
#endif


file_handler::~file_handler(){
  _delete_object_table();
  _delete_object_metadata();

  flush();
  close();
  _clear_error();

#if (_WIN64) || (_WIN32)
  DeleteCriticalSection(_object_mutex_ptr);
#endif
}


void file_handler::_clear_error(){
  if(!_last_error)
    return;

  __dm->delete_class_dbg(_last_error, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  _last_error = NULL;
}

void file_handler::_set_last_error(long long err_code, const std::string& err_msg){
  _clear_error();

  string_var _str = err_msg.c_str();
  _last_error = __dm->new_class_dbg<error_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, &_str, err_code);
}

#if (_WIN64) || (_WIN32)
void file_handler::_update_last_error_winapi(){
  DWORD _errcode = GetLastError();
  std::string _errmsg = get_windows_error_message(_errcode);
  _set_last_error(_errcode, _errmsg);
}
#endif


void file_handler::_initiate_reference(){
  // metadata table should use reference
  _lc.context->api_value->newtable(_lc.istate);
  _object_metadata = __dm->new_class_dbg<table_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, &_lc, -1);
  _lc.context->api_stack->pop(_lc.istate, 1);

  string_var _keystr = FILE_HANDLER_METADATA_OBJECT_POINTER;
  lightuser_var _pointer = this;
  _object_metadata->set_value((I_variant*)&_keystr, (I_variant*)&_pointer); // directly casting to not make the compiler upset
}

void file_handler::_delete_object_metadata(){
  if(_object_metadata){
    __dm->delete_class_dbg(_object_metadata, DYNAMIC_MANAGEMENT_DEBUG_DATA);
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
    __dm->delete_class_dbg(_object_table, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _object_table = NULL;
  }
}


void file_handler::_skip_str(bool(*filter_cb)(char ch)){
  if(already_closed()){
    _set_last_error(ERROR_HANDLE_EOF, "File already closed.");
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
    _set_last_error(ERROR_HANDLE_EOF, "File already closed.");
    return -1;
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
  if(_is_pipe_handle) 
    return;

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


std::string file_handler::_get_pointer_str(const void* pointer){
  return format_str_mem(__dm, "0x%X", pointer);
}


int file_handler::_lua_line_cb(const core* lc){
  file_handler* _this;
  int _arg_count = 0;
  int _ret_count = 0;


  error_var* _err = pstack_call_core<error_var*>(lc, 0, 0, [](const core* lc, file_handler** _pthis, int* _parg_count){
    lc->context->api_stack->pushvalue(lc->istate, lua_upvalueindex(1)); // get metadata
    if(lc->context->api_value->type(lc->istate, -1) != LUA_TTABLE){
      string_var _errmsg = "Function is not properly setup; Object table is nil.";
      return __dm->new_class_dbg<error_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, &_errmsg, -1);
    }

    lc->context->api_value->pushstring(lc->istate, FILE_HANDLER_METADATA_OBJECT_POINTER);
    lc->context->api_value->gettable(lc->istate, -2);
    if(lc->context->api_value->type(lc->istate, -1) != LUA_TLIGHTUSERDATA){
      string_var _errmsg = "Object already deleted.";
      return __dm->new_class_dbg<error_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, &_errmsg, -1);
    }

    (*_pthis) = (file_handler*)lc->context->api_value->touserdata(lc->istate, -1);
    lc->context->api_stack->pop(lc->istate, 2); // pop metadata table and object pointer

    lc->context->api_stack->pushvalue(lc->istate, lua_upvalueindex(2)); // get arg count
    if(lc->context->api_value->type(lc->istate, -1) != LUA_TNUMBER){
      string_var _errmsg = "Function is not properly setup; Arg count variable is not an int.";
      return __dm->new_class_dbg<error_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, &_errmsg, -1);
    }

    (*_parg_count) = lc->context->api_value->tointeger(lc->istate, -1);
    lc->context->api_stack->pop(lc->istate, 1); // pop arg count

    return (error_var*)NULL;
  }, lc, &_this, &_arg_count);

  if(_err){
    _err->get_error_data()->push_to_stack(lc);
    __dm->delete_class_dbg(_err, DYNAMIC_MANAGEMENT_DEBUG_DATA);

    lc->context->api_util->error(lc->istate);
  }


  vararr _args;
  vararr _results;

  for(int i = 1; i <= _arg_count; i++){
    I_variant* _arg = lc->context->api_varutil->to_variant(lc->istate, lua_upvalueindex(i+2));
    _args.append_var(_arg);
    lc->context->api_varutil->delete_variant(_arg);
  }

  read(_this, &_args, &_results);

  // NOTE: error throwing is not possible in read

  for(int i = 0; i < _results.get_var_count(); i++)
    _results.get_var(i)->push_to_stack(lc);

  _ret_count = _results.get_var_count();

  return _ret_count;
}


const I_error_var* file_handler::get_last_error() const{
  return _last_error;
}


bool file_handler::check_object_validity(const I_table_var* tvar){
  std::set<comparison_variant> _checks;

  // indexing fdata from file_handler
  int _idx = 0;
  while(_file_handler_fdata[_idx].name){
    string_var _str = _file_handler_fdata[_idx].name;
    _checks.insert(_str);
    
    _idx++;
  }

  const core* _tvar_core = tvar->get_lua_core();
  const I_variant** _key_list = tvar->get_keys();
  _idx = 0;
  while(_key_list[_idx]){
    const I_variant* _testvar = tvar->get_value(_key_list[_idx]);
    comparison_variant _comp = _key_list[_idx];
    auto _iter = _checks.find(_comp);

    if(!_testvar || _testvar->get_type() != I_function_var::get_static_lua_type())
      goto skip_check;

    if(_iter == _checks.end())
      goto skip_check;
    
    _checks.erase(_iter);

    skip_check:{}
    
    _idx++;

    // delete result from table_var
    if(_testvar)
      _tvar_core->context->api_varutil->delete_variant(_testvar);
  } 

  return !_checks.size();
}


void file_handler::close(I_object* obj, const I_vararr* args, I_vararr* res){
  file_handler* _this = dynamic_cast<file_handler*>(obj);
  _this->lock_object();
  _this->_clear_error();

  _this->close();

  if(_this->get_last_error())
    _this->_fill_error_with_last(res);

  _this->unlock_object();
}

void file_handler::flush(I_object* obj, const I_vararr* args, I_vararr* res){
  file_handler* _this = dynamic_cast<file_handler*>(obj);
  _this->lock_object();
  _this->_clear_error();

  _this->flush();

  if(_this->get_last_error())
    _this->_fill_error_with_last(res);
  
  _this->unlock_object();
}

void file_handler::lines(I_object* obj, const I_vararr* args, I_vararr* res){
  // don't store I_object pointer directly, if possible, use reference object like tables
  file_handler* _this = dynamic_cast<file_handler*>(obj);
  _this->lock_object();
  _this->_clear_error();

{ // enclosure for using gotos
  if(_this->already_closed()){
    _this->_set_last_error(ERROR_HANDLE_EOF, "File already closed.");
    res->append_var(_this->get_last_error());

    goto skip_to_return;
  }

  // in case of no argument (switch to default format)
  vararr _fargs;
  if(args->get_var_count() < 1){
    string_var _def_str = FILE_HANDLER_LINES_DEFAULT_VALUE1;
    _fargs.append_var(&_def_str);
  }
  else
    _fargs.append(args);

  // iterator function
  function_var _iterfunc(_lua_line_cb);
  
  // set metadata table as first upvalue
  _iterfunc.get_arg_closure()->append_var(_this->_object_metadata);
  _this->_lc.context->api_stack->pop(_this->_lc.istate, 1);

  // set how many values to read
  number_var _count = _fargs.get_var_count();
  _iterfunc.get_arg_closure()->append_var(&_count);

  // push this parameter to iterator function's upvalues
  _iterfunc.get_arg_closure()->append(&_fargs);

  // add iterator function as result
  res->append_var(&_iterfunc);
} // enclosure closing

  skip_to_return:{}
  _this->unlock_object();
}

void file_handler::read(I_object* obj, const I_vararr* args, I_vararr* res){
  // n: read number on the first word
  // a: read all data
  // l: read a line, without the newline character
  // L: read a line, with the newline character
  // (number): read a number of bytes
  file_handler* _this = dynamic_cast<file_handler*>(obj);
  _this->lock_object();
  _this->_clear_error();

  // returns nothing when eof, and close the file if the open_mode has open_automatic_close flag
  if(_this->end_of_file_reached()){
    if(_this->_op_code & open_automatic_close)
      _this->close();

    goto skip_to_return;
  }

  for(int i = 0; i < args->get_var_count(); i++){
{ // enclosure for using goto    
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

          char* _buffer = (char*)__dm->malloc(IO_MAXIMUM_ALL_READ_LEN+1, DYNAMIC_MANAGEMENT_DEBUG_DATA); // with null termination

          _this->_skip_str(_filter_cb); // (if available) skip the first whitespaces
          int _read_len = _this->_read_str(_buffer, IO_MAXIMUM_ALL_READ_LEN, _filter_cb);
          _this->_skip_str(_filter_cb_invert); // (if available) skip unread characters until whitespace
          _this->_skip_str(_filter_cb); // skip next whitespace

          number_var _num = atof(_buffer);
          res->append_var(&_num);

          __dm->free(_buffer, DYNAMIC_MANAGEMENT_DEBUG_DATA);
        }

        break; case 'a':{ // MARK: READ 'a'
          char* _buffer = (char*)__dm->malloc(IO_MAXIMUM_ALL_READ_LEN+1, DYNAMIC_MANAGEMENT_DEBUG_DATA); // with null termination
          int _read_len = _this->_read_str(_buffer, IO_MAXIMUM_ALL_READ_LEN);

          string_var _str(_buffer, _read_len);
          res->append_var(&_str);

          __dm->free(_buffer, DYNAMIC_MANAGEMENT_DEBUG_DATA);
        }

        break; case 'L':
        break; case 'l':{ // MARK: READ 'l' or 'L'
          bool (*_filter_cb)(char) = [](char c){return c == '\n' || c == '\r';};

          char* _buffer = (char*)__dm->malloc(IO_MAXIMUM_ALL_READ_LEN+2, DYNAMIC_MANAGEMENT_DEBUG_DATA); // with null termination also newline character (if possible)
          
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

          __dm->free(_buffer, DYNAMIC_MANAGEMENT_DEBUG_DATA);
        }

        break; default:
          goto on_error_format;
      }
    }
    else if(_iarg->get_type() == LUA_TNUMBER){ // MARK: READ based on a number
      const I_number_var* _number_var = dynamic_cast<const I_number_var*>(_iarg);
      char* _buffer = (char*)__dm->malloc(IO_MAXIMUM_ALL_READ_LEN+1, DYNAMIC_MANAGEMENT_DEBUG_DATA);

      size_t _read_len = (size_t)_number_var->get_number();
      if(_number_var->get_number() > 0)
        _read_len = _this->_read_str(_buffer, _read_len);
      else
        _read_len = 0;

      string_var _str(_buffer, _read_len);
      res->append_var(&_str);

      __dm->free(_buffer, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    }
    else
      goto on_error_format;

    // check for errors (will be skipped if current arguments are after the first)
    const I_error_var* _errvar = _this->get_last_error();
    if(i <= 0 && _errvar)
      goto on_error;

} // enclosure closing

    continue;
    on_error_eof:{
      nil_var _nvar;
      res->append_var(&_nvar);
    }
  }

  skip_to_return:{}
  _this->unlock_object();
  return;


  // ERROR GOTOs

  on_error_format:{
    _this->_set_last_error(ERROR_BAD_FORMAT, "Invalid format in file:read function.");
    goto on_error;
  }

  on_error:{
    res->clear();
    _this->_fill_error_with_last(res);
    _this->unlock_object();
  }
}

void file_handler::seek(I_object* obj, const I_vararr* args, I_vararr* res){
  file_handler* _this = dynamic_cast<file_handler*>(obj);
  _this->lock_object();
  _this->_clear_error();
  
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

  _this->unlock_object();
  return;


  // ERROR GOTOs

  on_error:{
    res->clear();
    _this->_fill_error_with_last(res);
    _this->unlock_object();
  }
}

void file_handler::setvbuf(I_object* obj, const I_vararr* args, I_vararr* res){
  file_handler* _this = dynamic_cast<file_handler*>(obj);
  _this->lock_object();
  _this->_clear_error();

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
  if(_this->get_last_error())
    goto on_error;

}// enclosure closing

  _this->unlock_object();
  return;


  // ERROR GOTOs

  on_error:{
    res->clear();
    _this->_fill_error_with_last(res);
    _this->unlock_object();
  }
}

void file_handler::write(I_object* obj, const I_vararr* args, I_vararr* res){
  file_handler* _this = dynamic_cast<file_handler*>(obj);
  _this->lock_object();
  _this->_clear_error();
  
{// enclosure for using gotos

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

  // add this object as result
  res->append_var(_this->_object_table);
  _this->unlock_object();
  return;
  

  // ERROR GOTOs
  
  on_error:{
    res->clear();
    _this->_fill_error_with_last(res);
    _this->unlock_object();
  }
}


bool file_handler::close(){
  bool _result = true;
  lock_object();
  _clear_error();

{ // enclosure for using gotos
#if (_WIN64) || (_WIN32)
  if(already_closed()){
    _set_last_error(ERROR_HANDLE_EOF, "File already closed.");
    _result = false;
    goto skip_to_return;
  }

  _delete_object_table();

  if(_own_handle){
    flush();
    CloseHandle(_hfile);
  }

  _hfile = NULL;
#endif
} // enclosure closing

  skip_to_return:{}
  unlock_object();
  return _result;
}

bool file_handler::flush(){
  bool _result = true;
  lock_object();
  _clear_error();

#if (_WIN64) || (_WIN32)
  if(already_closed()){ 
    _set_last_error(ERROR_HANDLE_EOF, "File already closed.");
    _result = false;
    goto skip_to_return;
  }

  FlushFileBuffers(_hfile);
#endif

  skip_to_return:{}
  unlock_object();
  return _result;
}

char file_handler::peek(){
  char _tmpc;
  lock_object();
  _clear_error();

{ // enclosure for using gotos
  if(already_closed()){
    _set_last_error(ERROR_HANDLE_EOF, "File already closed.");
    _tmpc = '\0';
    goto skip_to_return;
  }

  if(end_of_file_reached()){
    _tmpc = '\0';
    goto skip_to_return;
  }

#if (_WIN64) || (_WIN32)
  bool _success = PeekNamedPipe(_hfile, &_tmpc, sizeof(char), NULL, NULL, NULL);
  if(!_success)
    goto on_windows_error;
#endif
} // enclosure closing

  skip_to_return:{}
  unlock_object();
  return _tmpc;


  // ERROR GOTOs

#if (_WIN64) || (_WIN32)
  on_windows_error:{
    _update_last_error_winapi();
    unlock_object();
    return '\0';
  }
#endif
}

long file_handler::seek(seek_opt opt, long offset){
  long _result = -1;
  lock_object();
  _clear_error();

{ // enclosure for using goto
  if(_is_pipe_handle){
    _set_last_error(ERROR_INVALID_HANDLE, "Cannot seek a pipe handle.");
    goto skip_to_return;
  }

  if(already_closed()){
    _set_last_error(ERROR_HANDLE_EOF, "File already closed.");
    goto skip_to_return;
  }

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
} // enclosure closing

  skip_to_return:{}
  unlock_object();
  return _result;


  // ERROR GOTOs

#if (_WIN64) || (_WIN32)
  on_windows_error:{
    _update_last_error_winapi();
    unlock_object();
    return -1;
  }
#endif
}

size_t file_handler::read(char* buffer, size_t buffer_size){
  lock_object();
  _clear_error();
  size_t _result = _read_str(buffer,  buffer_size);
  unlock_object();
  return _result;
}

size_t file_handler::write(const char* buffer, size_t buffer_size){
  size_t _result = 0;
  lock_object();
  _clear_error();

{ // enclosure for using gotos
  if(already_closed()){
    _set_last_error(ERROR_HANDLE_EOF, "File already closed.");
    goto skip_to_return;
  }

  if(!can_write()){
    _set_last_error(ERROR_INVALID_HANDLE, "Write operation is prohibited.");
    goto skip_to_return;
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
} // enclosure closing

  skip_to_return:{}
  unlock_object();
  return _result;


  // ERROR GOTOs
#if (_WIN64) || (_WIN32)
  on_windows_error:{
    _update_last_error_winapi();
    unlock_object();
    return 0;
  }
#endif
}

void file_handler::set_buffering_mode(buffering_mode mode){
  lock_object();
  _clear_error();
  if(already_closed()){
    _set_last_error(ERROR_HANDLE_EOF, "File already closed.");
    goto skip_to_return;
  }

  _buffering_mode = mode;

  skip_to_return:{}
  unlock_object();
}


bool file_handler::already_closed() const{
  return !_hfile;  
}

bool file_handler::end_of_file_reached() const{
  return get_remaining_read() <= 0;
}

size_t file_handler::get_remaining_read() const{
  size_t _result = 0;
  lock_object();

{ // enclosure for using gotos
  if(already_closed())
    goto skip_to_return;

#if (_WIN64) || (_WIN32)
  DWORD _available_bytes = 0;
  PeekNamedPipe(_hfile, NULL, 0, NULL, &_available_bytes, NULL);
  _result = _available_bytes;
#endif
} // enclosure closing

  skip_to_return:{}
  unlock_object();
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

void file_handler::on_object_added(const core* lc){
  lock_object();
  _delete_object_table();

  string_var _keystr = FILE_HANDLER_LUA_OBJECT_TABLE;
  _object_table = __dm->new_class_dbg<table_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, lc, -1);
  _object_metadata->set_value((I_variant*)&_keystr, (I_variant*)_object_table); // directly casting to not make the compiler upset

  // Reset the lua_core to make sure to use this compilation context.
  _lc = *lc;
  unlock_object();
}


void file_handler::lock_object() const{
#if (_WIN64) || (_WIN32)
  EnterCriticalSection(_object_mutex_ptr);
#endif
}

void file_handler::unlock_object() const{
#if (_WIN64) || (_WIN32)
  LeaveCriticalSection(_object_mutex_ptr);
#endif
}




DLLEXPORT lua::library::I_io_handler* CPPLUA_LIBRARY_CREATE_IO_HANDLER(const lua::library::io_handler_api_constructor_data* data){
  return __dm->new_class_dbg<io_handler>(DYNAMIC_MANAGEMENT_DEBUG_DATA, data->lua_core, data->param);
}

DLLEXPORT void CPPLUA_LIBRARY_DELETE_IO_HANDLER(lua::library::I_io_handler* handler){
  __dm->delete_class_dbg(handler, DYNAMIC_MANAGEMENT_DEBUG_DATA);
}


DLLEXPORT lua::library::I_file_handler* CPPLUA_LIBRARY_CREATE_FILE_HANDLER(const lua::library::file_handler_api_constructor_data* data){
  if(!data->use_pipe){
    return __dm->new_class_dbg<file_handler>(DYNAMIC_MANAGEMENT_DEBUG_DATA, data->lua_core, data->path, data->open_mode);
  }
  else{
#if (_WIN64) || (_WIN32)
    return __dm->new_class_dbg<file_handler>(DYNAMIC_MANAGEMENT_DEBUG_DATA, data->lua_core, data->pipe_handle, data->is_output);
#endif
  }
}

DLLEXPORT void CPPLUA_LIBRARY_DELETE_FILE_HANDLER(lua::library::I_file_handler* handler){
  __dm->delete_class_dbg(handler, DYNAMIC_MANAGEMENT_DEBUG_DATA);
}


#endif // LUA_CODE_EXISTS