#include "../Src/lua_includes.h"
#include "../Src/lua_variant.h"
#include "../Src/luaruntime_handler.h"

using namespace lua;


int _nvalue = 0;


void _test_func(const I_vararr* args, I_vararr* res){
  printf("C++: This is a test.\n");
}

void _check_func(const I_vararr* args, I_vararr* res){
  if(args->get_var_count() < 1){
    string_var _strmsg = "Agument is not enough";
    error_var _errobj(&_strmsg, -1);
    res->append_var(&_errobj);

    return;
  }

  function_var* _fvar;
  {const I_variant* _check_var = args->get_var(0);
    if(_check_var->get_type() != I_function_var::get_static_lua_type()){
      string_var _strmsg = "Argument 1 is not a function.";
      error_var _errobj(&_strmsg, -1);
      res->append_var(&_errobj);

      return;
    }

    _fvar = (function_var*)cpplua_create_var_copy(_check_var);
  }

  printf("Check function\n");
  if(_fvar->is_cfunction()){
    printf("Is C function.\nUpvalues:\n");
    I_vararr* _upvalues = _fvar->get_arg_closure();
    for(int i = 0; i < _upvalues->get_var_count(); i++){
      string_store _str; _upvalues->get_var(i)->to_string(&_str);
      printf("\t%d. '%s'\n", i+1, _str.data.c_str());
    }

    number_var _next_var = _nvalue;
    _upvalues->append_var(&_next_var);

    _nvalue++;
  }
  else{
    printf("Is Lua function.\n");
  }

  res->append_var(_fvar);
}

void _modify_test_lua_func(){
  
}


int main(){
  runtime_handler* _runtime = new runtime_handler("TestSrc/test_reffunc.lua", false);
  const I_variant* _errcheck = _runtime->get_last_error_object();
  if(_errcheck){
    string_store _str; _errcheck->to_string(&_str);
    printf("Cannot open runtime_handler: %s\n", _str.data.c_str());

    exit(-1);
  }

  func_db* _fdb = _runtime->get_function_database();
  _fdb->expose_c_function_nonstrict("check_func", _check_func);
  _fdb->expose_c_function_nonstrict("test_func", _test_func);

  int _errcode = _runtime->run_current_file();
  if(_errcode != LUA_OK){
    _errcheck = _runtime->get_last_error_object();

    string_store _str; _errcheck->to_string(&_str);
    printf("Error happened in runtime: %s\n", _str.data.c_str());

    exit(-1);
  }

  delete _runtime;
  printf("Program finished.\n");
}