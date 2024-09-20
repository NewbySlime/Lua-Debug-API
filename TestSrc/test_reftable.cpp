#include "../Src/lua_includes.h"
#include "../Src/luaruntime_handler.h"

#include "iostream"



void _set_to(const lua::I_vararr* args, lua::I_vararr* res){
  if(args->get_var_count() < 3){
    lua::string_var _errmsg = "Not enough parameter.";
    lua::error_var _errvar = lua::error_var(&_errmsg, -1);
    res->append_var(&_errvar);

    return;
  }
  
  const lua::I_table_var* _tivar;
  {const lua::I_variant* _checkvar = args->get_var(0);
    if(_checkvar->get_type() != lua::I_table_var::get_static_lua_type()){
      lua::string_var _errmsg = "First value is not a table.";
      lua::error_var _errvar = lua::error_var(&_errmsg, -1);
      res->append_var(&_errvar);

      return;
    }

    _tivar = dynamic_cast<const lua::I_table_var*>(_checkvar); 
  }

  // create copy since it is constant
  lua::table_var* _tvar = lua::table_var::create_copy_static(_tivar);
  _tvar->set_value(args->get_var(1), args->get_var(2));
  cpplua_delete_variant(_tvar);
}

void _create_copy(const lua::I_vararr* args, lua::I_vararr* res){
  if(args->get_var_count() < 1){
    lua::string_var _errmsg = "Not enough parameter.";
    lua::error_var _errvar = lua::error_var(&_errmsg, -1);
    res->append_var(&_errvar);

    return;
  }

  const lua::I_table_var* _tivar;
  {const lua::I_variant* _checkvar = args->get_var(0);
    if(_checkvar->get_type() != lua::I_table_var::get_static_lua_type()){
      lua::string_var _errmsg = "First value is not a table.";
      lua::error_var _errvar = lua::error_var(&_errmsg, -1);
      res->append_var(&_errvar);

      return;
    }

    _tivar = dynamic_cast<const lua::I_table_var*>(_checkvar);
  }

  // create table copy using own compilation code
  lua::table_var* _tvarref = lua::table_var::create_copy_static(_tivar);
  lua::table_var* _tvarcopy = dynamic_cast<lua::table_var*>(_tvarref->create_table_copy());
  res->append_var(_tvarcopy);

  cpplua_delete_variant(_tvarref);
  cpplua_delete_variant(_tvarcopy);
}


int main(){
  using namespace lua;

  runtime_handler* _runtime = new runtime_handler("TestSrc/test_reftable.lua", false);
  const I_variant* _err = _runtime->get_last_error_object();
  if(_err){
    string_store _str;
    _err->to_string(&_str);

    std::cout << _str.data << std::endl;
    exit(-1);
  }

  func_db* _fdb = _runtime->get_function_database();
  _fdb->expose_c_function_nonstrict("set_to", _set_to);
  _fdb->expose_c_function_nonstrict("create_copy", _create_copy);

  int _err_code = _runtime->run_current_file();
  if(_err_code != LUA_OK){
    _err = _runtime->get_last_error_object();
    string_store _str;
    _err->to_string(&_str);

    std::cout << _str.data << std::endl;
    exit(_err_code);
  }

  std::cout << "Program finished." << std::endl;
  delete _runtime;
}