#include "dllutil.h"
#include "error_definition.h"
#include "error_util.h"
#include "luaapi_object_util.h"
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
#include "string.h"

#if (_WIN64) || (_WIN32)
#include "Windows.h"
#include "winerror.h"
#elif (__linux)
#include "unistd.h"
#include "sys/ioctl.h"
#endif

#include "defines.h"


#define IO_HANDLER_STDOUT_VARNAME "stdout"
#define IO_HANDLER_STDIN_VARNAME "stdin"
#define IO_HANDLER_STDERR_VARNAME "stderr"

#define TEMPORARY_FILE_EXT ".tmp"

#define FILE_HANDLER_REFERENCE_INTERNAL_DATA "__clua_file_handler_ref_data"
#define FILE_HANDLER_METADATA_OBJECT_POINTER "object"
#define FILE_HANDLER_LUA_OBJECT_TABLE "table_obj"

#define FILE_HANDLER_LINES_DEFAULT_VALUE1 "*l"

#define TEMPORARY_BUFFER_SIZE 1024


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
  fdata(NULL, NULL) // array closing
};


#define IO_HANDLER_REPLACE_DEF_FILE(def_file, file_open_mode) \
  vararr args, res; \
    _initialize_function_vararr(lc, &args, &res); \
  io_handler* _this = _get_object_by_closure(lc); \
  _this->lock_object(); \
  _this->_clear_error(); \
   \
{ /* enclosure for using goto */ \
   \
  /* Getting arguments */ \
  {const I_variant* _check_var; \
    if(args.get_var_count() < 1){ /* push default input file when argument is empty */ \
      res.clear(); \
      res.append_var(_this->def_file); \
      goto skip_to_return; \
    } \
     \
    _check_var = args.get_var(0); \
    switch(_check_var->get_type()){ \
      /* set default input file from table data */ \
      break; \
      case I_object_var::get_static_lua_type(): \
      case I_table_var::get_static_lua_type():{ \
        if(!file_handler::check_object_validity(lc, _check_var)){ \
          _this->_set_last_error(ERROR_BAD_ARGUMENTS, "Argument #1: File handle object is invalid."); \
          goto on_error; \
        } \
         \
        _check_var->push_to_stack(&_this->_lc); \
        _this->def_file->from_state(&_this->_lc, -1); \
        _this->_lc.context->api_stack->pop(_this->_lc.istate, 1); \
      } \
       \
      /* set default input file from file name */ \
      break; case I_string_var::get_static_lua_type():{ \
        const I_string_var* _strvar = dynamic_cast<const I_string_var*>(_check_var); \
        file_handler* _tmp_obj = __dm->new_class_dbg<file_handler>(DYNAMIC_MANAGEMENT_DEBUG_DATA, &_this->_lc, _strvar->get_string(), (file_open_mode)); \
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
        _this->_lc.context->api_stack->pop(_this->_lc.istate, 1); \
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
  skip_to_return:{} \
  _this->unlock_object(); \
  return _finish_function_vararr(lc, &args, &res); \
   \
   \
  /* ERROR GOTOs */ \
   \
  on_error:{ \
    _this->unlock_object(); \
    _this->get_last_error()->push_to_stack(lc); \
    lc->context->api_util->error(lc->istate); \
  } \
   \
  return 0; /* suppress error, error function will throw an error */

#define IO_HANDLER_REDIRECT_FUNCTION(table_obj, fname) \
  vararr args, res; \
    _initialize_function_vararr(lc, &args, &res); \
  io_handler* _this = _get_object_by_closure(lc); \
  _this->lock_object(); \
  _this->_clear_error(); \
   \
{ /* enclosure for using gotos */ \
   \
  vararr _fargs; \
    _fargs.append_var(table_obj); \
    _fargs.append(&args); \
  string_var _fname = fname; \
  I_function_var* _fvar = dynamic_cast<I_function_var*>(table_obj->get_value(_fname)); \
  if(!_fvar){ \
    _this->_set_last_error(-1, format_str("Cannot run an nil function (%s)", _fname.get_string()).c_str()); \
    goto skip_to_return; \
  } \
   \
  int _errcode = _fvar->run_function(&_this->_lc, &_fargs, &res); \
  if(_this->_has_error(&res)) \
    _this->_copy_error_from(&res); \
  else if(_errcode != LUA_OK) \
    _this->_set_last_error(_errcode, res.get_var(0)); \
  /* delete result from table_var */ \
  table_obj->free_variant(_fvar); \
   \
} /* enclosure closing */ \
   \
  skip_to_return:{}\
  _this->unlock_object(); \
  if(_this->get_last_error()){ \
    _this->get_last_error()->push_to_stack(lc); \
    lc->context->api_util->error(lc->istate); \
  } \
   \
  return _finish_function_vararr(lc, &args, &res);
  
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
  _lc.context->api_stack->pop(_lc.istate, 1); \
} /* enclosure closing */ \
  skip_to_return:{} \
  unlock_object(); \
  return _result; \



static void _def_iohandler_destructor(I_object* obj){
  __dm->delete_class_dbg(obj, DYNAMIC_MANAGEMENT_DEBUG_DATA);
}

io_handler::io_handler(const lua::api::core* lc, const constructor_param* param): function_store(_def_iohandler_destructor){
  set_function_data(_io_handler_fdata);

  _object_mutex_ptr = &_object_mutex;

  I_file_handler* _stdout = NULL;
  I_file_handler* _stdin = NULL;
  I_file_handler* _stderr = NULL;
  if(param){
    _stdout = param->stdout_file;
    _stdin = param->stdin_file;
    _stderr = param->stderr_file;
  }

  if(!_stdout){
    _stdout = __dm->new_class_dbg<file_handler>(DYNAMIC_MANAGEMENT_DEBUG_DATA, lc, stdout, true);
  }

  if(!_stdin){
    _stdin = __dm->new_class_dbg<file_handler>(DYNAMIC_MANAGEMENT_DEBUG_DATA, lc, stdin, false);
  }

  if(!_stderr){
    _stderr = __dm->new_class_dbg<file_handler>(DYNAMIC_MANAGEMENT_DEBUG_DATA, lc, stderr, true);
  }

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

  flush();
  close();

  __dm->delete_class_dbg(_stdout_object, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  __dm->delete_class_dbg(_stdin_object, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  __dm->delete_class_dbg(_stderr_object, DYNAMIC_MANAGEMENT_DEBUG_DATA);

  __dm->delete_class_dbg(_fileout_def, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  __dm->delete_class_dbg(_filein_def, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  __dm->delete_class_dbg(_fileerr_def, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  
  _clear_error();
}


void io_handler::_clear_error(){
  if(_last_error){
    __dm->delete_class_dbg(_last_error, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _last_error = NULL;
  }
}

void io_handler::_set_last_error(long long err_code, const I_variant* err_obj){
  string_store _str; err_obj->to_string(&_str);
  _set_last_error(err_code, _str.data);
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


void io_handler::_initialize_function_vararr(const lua::api::core* lc, I_vararr* args, I_vararr* res){
  args->clear();
  res->clear();

  int _toplength = lc->context->api_stack->gettop(lc->istate);
  for(int i = 0; i < _toplength; i++){
    I_variant* _arg = lc->context->api_varutil->to_variant(lc->istate, i+1);
    args->append_var(_arg);
    lc->context->api_varutil->delete_variant(_arg);
  }
}

int io_handler::_finish_function_vararr(const lua::api::core* lc, I_vararr* args, I_vararr* res){
  for(int i = 0; i < res->get_var_count(); i++)
    res->get_var(i)->push_to_stack(lc);

  return res->get_var_count();
}

io_handler* io_handler::_get_object_by_closure(const lua::api::core* lc){
  lc->context->api_stack->pushvalue(lc->istate, LUA_FUNCVAR_GET_UPVALUE(1));
  I_object* _obj = lc->context->api_objutil->get_object_from_table(lc->istate, -1);
  lc->context->api_stack->pop(lc->istate, 1);

  return dynamic_cast<io_handler*>(_obj);
}


const I_error_var* io_handler::get_last_error() const{
  return _last_error;
}


int io_handler::close(const lua::api::core* lc){
  vararr args, res;
    _initialize_function_vararr(lc, &args, &res);

  io_handler* _this = _get_object_by_closure(lc);
  _this->lock_object();
  _this->_clear_error();

{ // enclosure for using goto
  {const I_variant* _check_var;
    if(args.get_var_count() >= 1){ 
      _check_var = args.get_var(0);
      switch(_check_var->get_type()){
        break;
        case I_object_var::get_static_lua_type():
        case I_table_var::get_static_lua_type():{
          if(!file_handler::check_object_validity(lc, _check_var)){
            _this->_set_last_error(ERROR_BAD_ARGUMENTS, "Argument #1: Not a file handle; invalid object.");
            goto on_error;
          }

          const char* _strkey = "close";

          lc->context->api_value->pushstring(lc->istate, _strkey);
          lc->context->api_value->gettable(lc->istate, 1);
          
          // no way that it could fail after checking its validity
          function_var _fvar(lc, -1);
          lc->context->api_stack->pop(lc->istate, 1);

          vararr _args;{
            _args.append_var(_check_var);
          }
          vararr _res;

          _fvar.run_function(lc, &_args, &_res);
          res.clear();
          res.append(&_res);

          if(_this->_has_error(&_res))
            _this->_copy_error_from(&_res);
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
  return _finish_function_vararr(lc, &args, &res);

  on_error:{
    res.clear();
    _this->_fill_error_with_last(&res);

    _this->unlock_object();
    return _finish_function_vararr(lc, &args, &res);
  }
}

int io_handler::flush(const lua::api::core* lc){
  vararr args, res;
    _initialize_function_vararr(lc, &args, &res);

  io_handler* _this = _get_object_by_closure(lc);
  _this->lock_object();
  _this->_clear_error();

  _this->flush();

  if(_this->get_last_error()){
    res.clear();
    _this->_fill_error_with_last(&res);
  }

  _this->unlock_object();

  return _finish_function_vararr(lc, &args, &res);
}

int io_handler::open(const lua::api::core* lc){
  vararr args, res;
    _initialize_function_vararr(lc, &args, &res);

  io_handler* _this = _get_object_by_closure(lc);
  _this->lock_object();
  _this->_clear_error();

{ // enclosure for using goto

  std::string _fname_str;
  std::string _mode_str = "r";

  // Getting arguments
  {const I_variant* _check_var;
    if(args.get_var_count() < 1){
      _this->_set_last_error(ERROR_BAD_ARGUMENTS, "Not enough argument.");
      goto on_error;
    }

    // get file name
    _check_var = args.get_var(0);
    if(_check_var->get_type() != I_string_var::get_static_lua_type()){
      _this->_set_last_error(ERROR_BAD_ARGUMENTS, "Argument #1: Not a string.");
      goto on_error;
    }

    _fname_str = dynamic_cast<const I_string_var*>(_check_var)->get_string();
    
    // get mode
    if(args.get_var_count() < 2)
      goto skip_argument_parse;

    _check_var = args.get_var(1);
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
  res.append_var(&_tmpobj);

} // enclosure closing

  _this->unlock_object();
  return _finish_function_vararr(lc, &args, &res);


  on_error:{
    res.clear();
    _this->_fill_error_with_last(&res);

    _this->unlock_object();
    return _finish_function_vararr(lc, &args, &res);
  }
}

int io_handler::input(const lua::api::core* lc){
  IO_HANDLER_REPLACE_DEF_FILE(_filein_def, I_file_handler::open_read)
} 

int io_handler::output(const lua::api::core* lc){
  IO_HANDLER_REPLACE_DEF_FILE(_fileout_def, I_file_handler::open_write)
}

int io_handler::error(const lua::api::core* lc){
  IO_HANDLER_REPLACE_DEF_FILE(_fileerr_def, I_file_handler::open_write)
}

int io_handler::lines(const lua::api::core* lc){
  vararr args, res;
    _initialize_function_vararr(lc, &args, &res);

  io_handler* _this = _get_object_by_closure(lc);
  _this->lock_object();
  _this->_clear_error();

{ // enclosure for using goto
  table_var _target_obj;
  vararr _farg;

  // Getting arguments
  {const I_variant* _check_var;
    if(args.get_var_count() < 1){ // push iterator function from default input
      _target_obj = *_this->_filein_def;
      _farg.append_var(&_target_obj);

      goto skip_parse_argument;
    }

    _check_var = args.get_var(0);
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

    // invoke to self
    _farg.append_var(&_target_obj);
    for(int i = 1; i < args.get_var_count(); i++)
      _farg.append_var(args.get_var(i));
  }

  skip_parse_argument:{}

  string_var _fname = "lines";
  // no way that the function could fail
  I_function_var* _func = dynamic_cast<I_function_var*>(_target_obj.get_value(_fname));
  res.clear();

  int _errcode = _func->run_function(_target_obj.get_lua_core(), &_farg, &res);
  
  // delete result from table_var
  _target_obj.free_variant(_func);

  if(_errcode != LUA_OK){
    string_store _errmsg;
    if(res.get_var_count() >= 1)
      res.get_var(0)->to_string(&_errmsg);

    _this->_set_last_error(_errcode, _errmsg.data);
  }

} // enclosure closing

  _this->unlock_object();
  return _finish_function_vararr(lc, &args, &res);


  on_error:{
    _this->unlock_object();
    _this->get_last_error()->push_to_stack(lc);
    lc->context->api_util->error(lc->istate);
  }

  return 0; // suppress warning, error function will throw an error
}

int io_handler::read(const lua::api::core* lc){
  IO_HANDLER_REDIRECT_FUNCTION(_this->_filein_def, "read")
}

int io_handler::tmpfile(const lua::api::core* lc){
  vararr args, res;
    _initialize_function_vararr(lc, &args, &res);

  io_handler* _this = _get_object_by_closure(lc);
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
  res.append_var(&_tmpobj);
} // enclosure closing

  _this->unlock_object();
  return _finish_function_vararr(lc, &args, &res);


  on_error:{
    res.clear();
    _this->_fill_error_with_last(&res);
    
    _this->unlock_object();
    return _finish_function_vararr(lc, &args, &res);
  }
}

int io_handler::type(const lua::api::core* lc){
  vararr args, res;
    _initialize_function_vararr(lc, &args, &res);

  io_handler* _this = _get_object_by_closure(lc);
  _this->lock_object();
  _this->_clear_error();

{ // enclosure for using goto
  if(args.get_var_count() < 1)
    goto on_invalid;

  const I_variant* _check_var = args.get_var(0);
  if(!file_handler::check_object_validity(lc, _check_var))
    goto on_invalid;

{ // enclosure for scoping
  const string_var _case_closed = "closed file";
  const string_var _case_normal = "file";

  const char* _fname = "read";

  lc->context->api_value->pushstring(lc->istate, _fname);
  lc->context->api_value->gettable(lc->istate, 1);

  // already checked for validity
  function_var _fvar(lc, -1);
  lc->context->api_stack->pop(lc->istate, 1);

  vararr _farg;{
    _farg.append_var(_check_var);
    number_var _num = 0.f; _farg.append_var(&_num);
  }
  vararr _fres;

  int _errcode = _fvar.run_function(&_this->_lc, &_farg, &_fres);
  if(_errcode != LUA_OK || _this->_has_error(&_fres))
    res.append_var(&_case_closed);
  else
    res.append_var(&_case_normal);

} // enclosure closing

} // enclosure closing

  _this->unlock_object();
  return _finish_function_vararr(lc, &args, &res);

  on_invalid:{
    res.clear();
    nil_var _nvar;
    res.append_var(&_nvar);

    _this->unlock_object();
    return _finish_function_vararr(lc, &args, &res);
  }
}

int io_handler::write(const lua::api::core* lc){
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
    vararr _fargs;
    vararr _fres;
    string_var _fname = "close";
    I_function_var* _fvar = dynamic_cast<I_function_var*>(_fileout_def->get_value(_fname));
    if(!_fvar){
      _set_last_error(-1, format_str("Cannot run an nil function (%s)", _fname.get_string()).c_str());
      goto skip_running_function;
    }

    _fvar->run_function(&_lc, &_fargs, &_fres);

    skip_running_function:{}

    if(_has_error(&_fres)) // check if returns an error data
      _copy_error_from(&_fres);

    if(_fvar)
      _fileout_def->free_variant(_fvar);
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
  I_function_var* _fvar = dynamic_cast<I_function_var*>(_fileout_def->get_value(_fname));
  if(!_fvar){
    _set_last_error(-1, format_str("Cannot run an nil function (%s) on output file.", _fname.get_string()).c_str());
    goto output_skip_process;
  }
  
  _fvar->run_function(&_lc, &_fargs, &_fres);

  output_skip_process:{}

  if(_has_error(&_fres)) // check if returns an error data
    _copy_error_from(&_fres);

  if(_fvar)
    _fileout_def->free_variant(_fvar);

  _fres.clear();
  
  // Run flush to error
  _fvar = dynamic_cast<I_function_var*>(_fileerr_def->get_value(_fname));
  if(!_fvar){
    _set_last_error(-1, format_str("Cannot run an nil function (%s) on error file.", _fname.get_string()).c_str());
    goto error_skip_process;
  }

  _fvar->run_function(&_lc, &_fargs, &_fres);

  error_skip_process:{}
  
  if(_has_error(&_fres)) // check if returns an error data
    _copy_error_from(&_fres);

  if(_fvar)
    _fileerr_def->free_variant(_fvar);
  
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

  vararr _fargs;
  vararr _fres;
  size_t _read_idx = 0;
  string_var _fname = "read";
  I_function_var* _fvar = dynamic_cast<I_function_var*>(_filein_def->get_value(_fname));
  if(!_fvar){
    _set_last_error(-1, format_str("Cannot run an nil function (%s) on input file.", _fname.get_string()).c_str());
    goto skip_to_return;
  }

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

  skip_to_return:{}

  if(_has_error(&_fres)) // check if the function returns an error data
    _copy_error_from(&_fres);

  if(_fvar)
    _filein_def->free_variant(_fvar);

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
  I_function_var* _fvar_write = dynamic_cast<I_function_var*>(_fileout_def->get_value(_fname_write));
  I_function_var* _fvar_seek = dynamic_cast<I_function_var*>(_fileout_def->get_value(_fname_seek));

  const I_variant* _check_var;

  size_t _fp_begin = 0;
  size_t _fp_end = 0;

  vararr _fargs_seek;{
    string_var _seek_mode = "set"; _fargs_seek.append_var(&_seek_mode);
  }

  vararr _fargs;
  vararr _fres;

{ // enclosure for using gotos
  if(!_fvar_write){
    _set_last_error(-1, format_str("Cannot run an nil function (%s) on output file.", _fname_write.get_string()).c_str());
    goto skip_to_return;
  }

  if(!_fvar_seek){
    _set_last_error(-1, format_str("Cannot run an nil function (%s) on output file.", _fname_seek.get_string()).c_str());
    goto skip_to_return;
  }

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
} // enclosure closing

  skip_to_return:{}

  if(_has_error(&_fres)) // check if previous function returns an error data
    _copy_error_from(&_fres);
  
  if(_fvar_write)
    _fileout_def->free_variant(_fvar_write);
  if(_fvar_seek)
    _fileout_def->free_variant(_fvar_seek);

  unlock_object();
  return _fp_end - _fp_begin;
}


I_debuggable_object* io_handler::as_debug_object(){
  return NULL;
}

void io_handler::on_object_added(const core* lc){
  lock_object();
  _obj_data = __dm->new_class_dbg<object_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, lc, -1);

  // set stdout
  lc->context->api_value->pushstring(lc->istate, IO_HANDLER_STDOUT_VARNAME);
  _stdout_object->push_to_stack(lc);
  lc->context->api_value->settable(lc->istate, -3);

  // set stdin
  lc->context->api_value->pushstring(lc->istate, IO_HANDLER_STDIN_VARNAME);
  _stdin_object->push_to_stack(lc);
  lc->context->api_value->settable(lc->istate, -3);

  // set stderr
  lc->context->api_value->pushstring(lc->istate, IO_HANDLER_STDERR_VARNAME);
  _stderr_object->push_to_stack(lc);
  lc->context->api_value->settable(lc->istate, -3);

  vararr _closure_data;
    _closure_data.append_var(_obj_data);

#define IOH_ON_OBJECT_ADDED_PUSH_FUNCTION(func_name) \
  {function_var _func_var(io_handler::func_name, &_closure_data); \
    lc->context->api_value->pushstring(lc->istate, #func_name); \
    _func_var.push_to_stack(lc); \
    lc->context->api_value->settable(lc->istate, -3); \
  }

  IOH_ON_OBJECT_ADDED_PUSH_FUNCTION(close)
  IOH_ON_OBJECT_ADDED_PUSH_FUNCTION(flush)
  IOH_ON_OBJECT_ADDED_PUSH_FUNCTION(open)
  IOH_ON_OBJECT_ADDED_PUSH_FUNCTION(input)
  IOH_ON_OBJECT_ADDED_PUSH_FUNCTION(output)
  IOH_ON_OBJECT_ADDED_PUSH_FUNCTION(error)
  IOH_ON_OBJECT_ADDED_PUSH_FUNCTION(lines)
  IOH_ON_OBJECT_ADDED_PUSH_FUNCTION(read)
  IOH_ON_OBJECT_ADDED_PUSH_FUNCTION(tmpfile)
  IOH_ON_OBJECT_ADDED_PUSH_FUNCTION(type)
  IOH_ON_OBJECT_ADDED_PUSH_FUNCTION(write)

  // Reset the lua_core to make sure to use this compilation context.
  _lc = *lc;
  unlock_object();
}


void io_handler::lock_object() const{
  _object_mutex_ptr->lock();
}

void io_handler::unlock_object() const{
  _object_mutex_ptr->unlock();
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

  _object_mutex_ptr = &_object_mutex;

  _lc = *lc;

  _op_code = op;
  _is_pipe_handle = false;
  _own_handle = true;
  _file_path = path;

{ // enclosure for using gotos
  if(op & open_temporary){
#if (__linux)
    const char* _template_name = "luaapi_XXXXXX";
    char* _template_name_buf = (char*)__dm->malloc(strlen(_template_name)+1, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _template_name_buf[strlen(_template_name)] = '\0';

    int _tmpfile_fd = mkstemp(_template_name_buf);
    _file_object = fdopen(_tmpfile_fd, "w+r");

    _file_path = _template_name_buf;
    __dm->free(_template_name_buf, DYNAMIC_MANAGEMENT_DEBUG_DATA);
#else
    _file_object = tmpfile();
    _file_path = "";
#endif

    if(!_file_object){
      _set_last_error(LUA_ERRFILE, format_str_mem(__dm, "Cannot open temporary file, err: %s", strerror(errno)));
      return;
    }

    _op_code = open_temporary;
    goto skip_file_creation;
  }

  _file_path = path;

  std::string _open_mode_str;
  if(op & open_append)
    _open_mode_str += "a";
  else
    _open_mode_str += op & open_write? "w": "r";

  if(op & open_binary)
    _open_mode_str += "b";

  if(op & open_special)
    _open_mode_str += "+";

  _file_object = fopen(path.c_str(), _open_mode_str.c_str());
  if(!_file_object){
    _set_last_error(LUA_ERRFILE, format_str_mem(__dm, "Cannot open file '%s', opening operation: %d, err: %s", path.c_str(), op, strerror(errno)));
    return;
  }
} // enclosure closing
 
  skip_file_creation:{}

  _initiate_reference();
}

file_handler::file_handler(const core* lc, FILE* pipe_handle, bool is_output): function_store(_def_fhandler_destructor){
  _initialize_file_handler_static_vars();
  set_function_data(_file_handler_fdata);

  _object_mutex_ptr = &_object_mutex;

  _lc = *lc;
  _file_object = pipe_handle;
  _is_pipe_handle = true;
  _own_handle = false;

  _op_code = is_output? open_write: open_read;

  _initiate_reference();

  _temporary_buffer.reserve(TEMPORARY_BUFFER_SIZE);
}

#if (_WIN64) || (_WIN32)
file_handler::file_handler(const core* lc, HANDLE pipe_handle, bool is_output): function_store(_def_fhandler_destructor){
  // TODO set error when the handle is not actually a pipe

  _initialize_file_handler_static_vars();
  set_function_data(_file_handler_fdata);

  _object_mutex_ptr = &_object_mutex;

  _lc = *lc;
  _hfile = pipe_handle;
  _is_pipe_handle = true;
  _is_pipe_handle_os_specific = true;
  _own_handle = false;

  _op_code = is_output? open_write: open_read;

  _initiate_reference();

  _temporary_buffer.reserve(TEMPORARY_BUFFER_SIZE);
}
#elif (__linux)
file_handler::file_handler(const core* lc, int pipe_handle, bool is_output): function_store(_def_fhandler_destructor){
  _initialize_file_handler_static_vars();
  set_function_data(_file_handler_fdata);

  _object_mutex_ptr = &_object_mutex;

  _lc = *lc;
  _pipe_handle = pipe_handle;
  _pipe_handle_valid = true;
  _is_pipe_handle = true;
  _is_pipe_handle_os_specific = true;
  _own_handle = false;

  _op_code = is_output? open_write: open_read;

  _initiate_reference();
  
  _temporary_buffer.reserve(TEMPORARY_BUFFER_SIZE);
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

  __dm->delete_class_dbg(_last_error, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  _last_error = NULL;
}

void file_handler::_set_last_error(long long err_code, const std::string& err_msg){
  _clear_error();

  string_var _str = err_msg.c_str();
  _last_error = __dm->new_class_dbg<error_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, &_str, err_code);
}

void file_handler::_update_last_ferror(){
  _set_last_error(errno, format_str_mem(__dm, "Error occurred when operating on a file. Err: %s", strerror(errno)));
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


void file_handler::_skip_str(char_filter_function filter_cb){
  char_filter_function _filter_func;
  if(filter_cb){
    _filter_func = [&](char ch){
      return !filter_cb(ch);
    };
  }

  _read_str(NULL, SIZE_MAX, _filter_func, true);
}

int file_handler::_read_str(char* buffer, size_t buflen, char_filter_function filter_cb, bool skip_if_empty){
  if(already_closed()){
    _set_last_error(ERROR_HANDLE_EOF, "File already closed.");
    return -1;
  }

  // returns when eof, and close the file if the open_mode has open_automatic_close flag
  if(end_of_file_reached()){
    if(_op_code & open_automatic_close)
      close();

    return -1;
  }

  if(!can_read()){
    _set_last_error(ERROR_INVALID_FUNCTION, "Read operation is prohibited.");
    return -1;
  }

  // for dummy read
  if(!buffer && (!buflen || !filter_cb))
    return 0;

  size_t _cur_idx = 0;
  char _tmpc;

  while(!end_of_file_reached() && _cur_idx < buflen){
    bool _using_temporary_buffer = false;
    if(_temporary_buffer.size()){
      _using_temporary_buffer = true;
      _tmpc = _temporary_buffer[0];
      _temporary_buffer.erase(_temporary_buffer.begin());
    }
    else{
      size_t _remaining_len = get_remaining_read();
      if(!_remaining_len && skip_if_empty)
        break;

      if(_is_pipe_handle && _is_pipe_handle_os_specific){
#if (_WIN64) || (_WIN32)
        // Was planning on using PeekNamedPipe, but for the sake of simplicity...
        if(!ReadFile(_hfile, &_tmpc, 1, NULL, NULL))
          goto on_windows_error;
#elif (__linux)
        if(!::read(_pipe_handle, &_tmpc, 1))
          goto on_error;
#else
#error No implementation
#endif
      }
      else{
        // stdio pipes will still use fread
        if(!fread(&_tmpc, 1, 1, _file_object))
          goto on_error;
      }
    }

    if(filter_cb && filter_cb(_tmpc)){
      if(_is_pipe_handle){
        if(_using_temporary_buffer)
          _temporary_buffer.insert(_temporary_buffer.begin(), _tmpc);
        else
          _temporary_buffer += _tmpc;
      }
      else
        fseek(_file_object, ftell(_file_object)-1, SEEK_SET);
      
      break;
    }

    if(buffer)
      buffer[_cur_idx] = _tmpc;
    
    _cur_idx++;
  }

  if(buffer)
    buffer[_cur_idx] = '\0';
  
  return _cur_idx;


  // ERROR GOTOs
#if (_WIN64) || (_WIN32)
  on_windows_error:{}
  _update_last_error_winapi();
  return 0;
#endif

  on_error:{}
  _update_last_ferror();
  return 0;
}


void file_handler::_fill_error_with_last(I_vararr* res){
  nil_var _nil;
  res->append_var(&_nil);

  if(_last_error)
    res->append_var(_last_error);
}


std::string file_handler::_get_pointer_str(const void* pointer){
  return format_str_mem(__dm, "0x%X", pointer);
}


int file_handler::_lua_line_cb(const core* lc){
  file_handler* _this;
  int _arg_count = 0;
  int _ret_count = 0;


  error_var* _err = pstack_call_core<error_var*>(lc, 0, 0, [](const core* lc, file_handler** _pthis, int* _parg_count){
    lc->context->api_stack->pushvalue(lc->istate, LUA_FUNCVAR_GET_UPVALUE(1)); // get metadata
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

    lc->context->api_stack->pushvalue(lc->istate, LUA_FUNCVAR_GET_UPVALUE(2)); // get arg count
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
    I_variant* _arg = lc->context->api_varutil->to_variant(lc->istate, LUA_FUNCVAR_GET_UPVALUE(i+2));
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


bool file_handler::check_object_validity(const core* lc, const I_variant* var){
  switch(var->get_type()){
    break;
      case I_table_var::get_static_lua_type():
      case I_object_var::get_static_lua_type():
        break;

    break; default:
      return false;
  }

  std::set<comparison_variant> _checks;

  // indexing fdata from file_handler
  int _idx = 0;
  while(_file_handler_fdata[_idx].name){
    string_var _str = _file_handler_fdata[_idx].name;
    _checks.insert(_str);
    
    _idx++;
  }

  var->push_to_stack(lc);
  int _table_idx = lc->context->api_stack->absindex(lc->istate, -1);
  
  lc->context->api_value->pushnil(lc->istate);
  while(lc->context->api_value->next(lc->istate, _table_idx)){
    I_variant* _keyvar = lc->context->api_varutil->to_variant(lc->istate, -2);
    comparison_variant _keyvar_cmp = _keyvar;
    auto _iter = _checks.find(_keyvar_cmp);
    if(_iter != _checks.end())
      _checks.erase(_iter);

    lc->context->api_varutil->delete_variant(_keyvar);
    lc->context->api_stack->pop(lc->istate, 1);
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
  vararr _new_args;
  _this->lock_object();
  _this->_clear_error();

  if(_this->already_closed()){
    _this->_set_last_error(-1, "File already closed.");
    goto on_error_do_throw;
  }

  // dummy read
  _this->_read_str(NULL, 0);
  if(_this->get_last_error())
    goto on_error;

  // switch to default argument
  if(args->get_var_count() <= 0){
    string_var _str = "l";
    _new_args.append_var(&_str);

    args = &_new_args;
  }

  for(int i = 0; i < _new_args.get_var_count(); i++){
{ // enclosure for using goto    
    if(_this->end_of_file_reached())
      goto on_error_eof;

    // skip reading if error already present from the previous argument
    if(_this->get_last_error())
      goto on_error_eof;

    const I_variant* _iarg = _new_args.get_var(i); 
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
    return;
  }

  on_error_do_throw:{
    _this->unlock_object();
    _this->get_last_error()->push_to_stack(&_this->_lc);
    _this->_lc.context->api_util->error(_this->_lc.istate);
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

  if(_this->already_closed()){
    _this->_set_last_error(-1, "File already closed.");
    goto on_error_do_throw;
  }
  
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
    return;
  }

  on_error_do_throw:{
    _this->unlock_object();
    _this->get_last_error()->push_to_stack(&_this->_lc);
    _this->_lc.context->api_util->error(_this->_lc.istate);
  }
}


bool file_handler::close(){
  bool _result = true;
  lock_object();
  _clear_error();

{ // enclosure for using gotos
  if(already_closed()){
    _set_last_error(ERROR_HANDLE_EOF, "File already closed.");
    _result = false;
    goto skip_to_return;
  }

  _delete_object_table();

  if(_own_handle){
    flush();
    fclose(_file_object);
  }

  _file_object = NULL;

#if (_WIN64) || (_WIN32)
  _hfile = NULL;
#elif (__linux)
  _pipe_handle_valid = false;
#endif

} // enclosure closing

  skip_to_return:{}
  unlock_object();
  return _result;
}

bool file_handler::flush(){ 
  if(_is_pipe_handle)
    return false;

  bool _result = true;
  lock_object();
  _clear_error();

  if(already_closed()){ 
    _set_last_error(ERROR_HANDLE_EOF, "File already closed.");
    _result = false;
    goto skip_to_return;
  }

  fflush(_file_object);

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

  if(_is_pipe_handle){
    if(_temporary_buffer.size())
      _tmpc = _temporary_buffer[0];
    else if(_is_pipe_handle_os_specific){
#if (_WIN64) || (_WIN32)
      if(!PeekNamedPipe(_hfile, &_tmpc, sizeof(char), NULL, NULL, NULL))
        goto on_windows_error;
#elif (__linux)
      if(!::read(_pipe_handle, &_tmpc, 1))
        goto on_file_error;

      _temporary_buffer += _tmpc;
#else
#error No implementation
#endif
    }
    else{
      if(!fread(&_tmpc, 1, 1, _file_object))
        goto on_file_error;

      _temporary_buffer += _tmpc;
    }
  }
  else{
    // not same as expected read len means error
    if(!fread(&_tmpc, 1, 1, _file_object))
      goto on_file_error;
    
    // nonzero means error
    if(fseek(_file_object, ftell(_file_object)-1, SEEK_SET))
      goto on_file_error;
  }
} // enclosure closing

  skip_to_return:{}
  unlock_object();
  return _tmpc;


  // ERROR GOTOs

#if (_WIN64) || (_WIN32)
  on_windows_error:{}
  _update_last_error_winapi();
  unlock_object();
  return '\0';
#endif

  on_file_error:{}
  _update_last_ferror();
  unlock_object();
  return '\0';
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

  long _current_idx = ftell(_file_object);
  // nonzero means error
  if(fseek(_file_object, 0, SEEK_END))
    goto on_file_error;

  long _file_end_idx = ftell(_file_object);
  // nonzero means error
  if(fseek(_file_object, _current_idx, SEEK_SET))
    goto on_file_error;

  long _finish_idx;
  switch(opt){
    break; case seek_begin:
      _finish_idx = 0;
    
    break; case seek_current:
      _finish_idx = _current_idx;
    
    break; case seek_end:
      _finish_idx = _file_end_idx;
  }

  _finish_idx += offset;
  _finish_idx = std::max((long)0, _finish_idx);
  _finish_idx = std::min(_file_end_idx, _finish_idx);

  // nonzere means error
  if(fseek(_file_object, _finish_idx, SEEK_SET))
    goto on_file_error;

  _result = _finish_idx;
} // enclosure closing

  skip_to_return:{}
  unlock_object();
  return _result;


  // ERROR GOTOs

  on_file_error:{}
  _update_last_ferror();
  unlock_object();
  return -1;
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

  bool _skip_stdio_write = false;
  if(_is_pipe_handle && _is_pipe_handle_os_specific){
#if (_WIN64) || (_WIN32)
    _skip_stdio_write = true;
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
#elif (__linux)
    _skip_stdio_write = true;
    _result = ::write(_pipe_handle, buffer, buffer_size);
    if(_result != buffer_size)
      goto on_file_error;
#else
#error No implementation
#endif
  }

  if(!_skip_stdio_write){
    _result = fwrite(buffer, 1, buffer_size, _file_object); 
    if(_result != buffer_size)
      goto on_file_error;
  }
} // enclosure closing

  skip_to_return:{}
  unlock_object();
  return _result;


  // ERROR GOTOs
#if (_WIN64) || (_WIN32)
  on_windows_error:{}
  _update_last_error_winapi();
  unlock_object();
  return 0;
#endif

  on_file_error:{}
  _update_last_ferror();
  unlock_object();
  return 0;
}

void file_handler::set_buffering_mode(buffering_mode mode){
  lock_object();
  _clear_error();

{ // enclosure for using gotos
  if(_is_pipe_handle){
    _set_last_error(ERROR_INVALID_PARAMETER, "Cannot set buffering mode to a pipe.");
    goto skip_to_return;
  }

  if(already_closed()){
    _set_last_error(ERROR_HANDLE_EOF, "File already closed.");
    goto skip_to_return;
  }

  int _actual_mode = _IOFBF;
  switch(mode){
    break; case buffering_line:
      _actual_mode = _IOLBF;
    
    break; case buffering_none:
      _actual_mode = _IONBF;
  }

  int _error_code = ::setvbuf(_file_object, NULL, _actual_mode, 0);
  if(_error_code){
    _update_last_ferror();
    goto skip_to_return;
  }
} // enclosure closing

  skip_to_return:{}
  unlock_object();
}


bool file_handler::already_closed() const{
  if(_is_pipe_handle && _is_pipe_handle_os_specific){
#if (_WIN64) || (_WIN32)
    if(!_hfile)
      return true;

    char _tmp;
    if(_op_code & open_write)
      return !WriteFile(_hfile, &_tmp, 0, NULL, NULL);
    else
      return !PeekNamedPipe(_hfile, &_tmp, sizeof(char), NULL, NULL, NULL);
#elif (__linux)
    return !_pipe_handle_valid;
#else
#error No implementation
#endif
  }

  return !_file_object;
}

bool file_handler::end_of_file_reached() const{
  if(_is_pipe_handle)
    return already_closed();

  return get_remaining_read() <= 0;
}

size_t file_handler::get_remaining_read() const{
  size_t _result = 0;
  lock_object();

{ // enclosure for using gotos
  if(!can_read())
    goto skip_to_return;

  if(already_closed())
    goto skip_to_return;

  bool _skip_stdio_check = false;
  if(_is_pipe_handle){
    _skip_stdio_check = true;
    if(_is_pipe_handle_os_specific){
#if (_WIN64) || (_WIN32)
      DWORD _available_bytes = 0;
      PeekNamedPipe(_hfile, NULL, 0, NULL, &_available_bytes, NULL);
      _result = _available_bytes;
#elif (__linux)
      int _size = 0;
      ioctl(_pipe_handle, FIONREAD, &_size);
      _result = _size;
#else
#error No implementation
#endif
    }
    else{
      // Since pipes used within FILE* only exist for stdio, this can be translated to each individual OSs pipes/file descriptors
#if (_WIN64) || (_WIN32)
      DWORD _available_bytes = 0;
      PeekNamedPipe(GetStdHandle(STD_INPUT_HANDLE), NULL, 0, NULL, &_available_bytes, NULL);
      _result = _available_bytes;
#elif (__linux)
      int _size = 0;
      ioctl(STDIN_FILENO, FIONREAD, &_size);
      _result = _size;
#else
#error No implementation
#endif
    }
  }

  if(!_skip_stdio_check){
    long _current_idx = ftell(_file_object);
    fseek(_file_object, 0, SEEK_END);
    _result = ftell(_file_object);
    fseek(_file_object, _current_idx, SEEK_SET);
  }
} // enclosure closing

  skip_to_return:{}
  unlock_object();
  return _result;
}


bool file_handler::can_write() const{
  return (_op_code & (open_write | open_append)) > 0 || (_op_code & open_special) > 0;
}

bool file_handler::can_read() const{
  return (_op_code & open_write) <= 0 || (_op_code & open_special) > 0;
}


I_debuggable_object* file_handler::as_debug_object(){
  return this;
}

void file_handler::on_object_added(const core* lc){
  lock_object();
  _delete_object_table();

  string_var _keystr = FILE_HANDLER_LUA_OBJECT_TABLE;
  _object_table = __dm->new_class_dbg<object_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, lc, -1);
  _object_metadata->set_value((I_variant*)&_keystr, (I_variant*)_object_table); // directly casting to not make the compiler upset

  // Reset the lua_core to make sure to use this compilation context.
  _lc = *lc;
  unlock_object();
}


void file_handler::lock_object() const{
  _object_mutex_ptr->lock();
}

void file_handler::unlock_object() const{
  _object_mutex_ptr->unlock();
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
#elif (__linux)
    return __dm->new_class_dbg<file_handler>(DYNAMIC_MANAGEMENT_DEBUG_DATA, data->lua_core, data->pipe_handle, data->is_output);
#endif
  }
}

DLLEXPORT void CPPLUA_LIBRARY_DELETE_FILE_HANDLER(lua::library::I_file_handler* handler){
  __dm->delete_class_dbg(handler, DYNAMIC_MANAGEMENT_DEBUG_DATA);
}


#endif // LUA_CODE_EXISTS