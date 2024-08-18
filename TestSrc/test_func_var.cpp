#include "../Src/lua_includes.h"
#include "../Src/lua_str.h"
#include "../Src/luadebug_hookhandler.h"
#include "../Src/luafunction_database.h"
#include "../Src/luatable_util.h"
#include "../Src/strutil.h"

#include "chrono"
#include "exception"
#include "stdexcept"
#include "string"
#include "thread"

#include "stdio.h"


using namespace lua;


void _iter_callback(lua_State* state, int key_idx, int value_idx, int iter_idx, void* cb_data){
  printf("Global I%d %s-%s\n", iter_idx, lua_typename(state, lua_type(state, key_idx)), lua_typename(state, lua_type(state, value_idx)));
}

void _print_program_state(lua_State* _state){
  lua_getglobal(_state, "program_state");
  std::string _global_str = lua::to_string(_state, -1);
  printf("Program State: %s\n", _global_str.c_str());
  lua_pop(_state, 1);
}


lua_State* _state;
lua::debug::hook_handler* _hook_handler;


void _hookcb(lua_State* state, void* cbdata){
  const lua_Debug* _dbg = _hook_handler->get_current_debug_value();
  if(!_dbg)
    return;

  switch(_dbg->event){
    break; case LUA_HOOKCALL:{
      printf("function called %s\n", _dbg->name);
    }

    break; case LUA_HOOKLINE:{
      printf("current line %d\n", _dbg->currentline);
    }
  }
}


lua::number_var _called_func(lua::variant* i1, lua::variant* i2){
  lua::number_var _result;
  if(i1->get_type() != LUA_TNUMBER){
    printf("[C++] i1 should be a number.\n");
    return _result;
  }

  if(i2->get_type() != LUA_TNUMBER){
    printf("[C++] i2 should be a number.\n");
    return _result;
  }

  lua::number_var* i1_n = (lua::number_var*)i1;
  lua::number_var* i2_n = (lua::number_var*)i2;

  _result = (*i1_n) * (*i2_n);
  printf("[C++] Function called from lua. Multiplying %f * %f = %f.\n", i1_n->get_number(), i2_n->get_number(), _result.get_number());

  return _result;
}


int main(){
  const char* _filepath = "./Test.lua";

  _state = luaL_newstate();
  luaL_openlibs(_state);

  {
    _hook_handler = new lua::debug::hook_handler(_state);
    _hook_handler->set_hook(LUA_MASKCALL, _hookcb, NULL);

    lua_Debug _debug_lua;
    lua::func_db _f_db = lua::func_db(_state);

    lua::lightuser_var _test_user = &_f_db;
    _test_user.setglobal(_state, "test_lud");

    lua_getglobal(_state, "test_lud");
    std::string _test_str = lua::to_string(_state, -1);
    lua_pop(_state, 1);

    lua::variant* _test_user_from = lua::to_variant_fglobal(_state, "test_lud");
    std::string _test_str_from = _test_user_from->to_string();
    delete _test_user_from;


    printf("[C++] Script launching...\n");
    long _result = luaL_dofile(_state, _filepath);
    try{
      if(_result != LUA_OK){
        lua::variant* _var = lua::to_variant(_state, -1);
        std::string _err_str = _var->to_string();

        delete _var;
        throw new std::runtime_error(format_str("Cannot open Script. (%d) %s", _result, _err_str.c_str()));
      }
      
      printf("[C++] Script launched.\n");

      _f_db.expose_lua_function<lua::nil_var, lua::number_var>("fill_program_state");
      _f_db.expose_lua_function<lua::number_var, lua::number_var, lua::number_var>("calculate_add2");
      _f_db.expose_lua_function<lua::nil_var>("call_c_function");

      _f_db.expose_c_function("_called_func", _called_func);

      lua::number_var _v1 = 1;
      lua::number_var _v2 = 10;

      {lua::variant* _number = _f_db.call_lua_function("calculate_add2", &_v1, &_v2);
        std::string _return_str = _number->to_string();

        if(_number->get_type() == LUA_TERROR)
          printf("[C++] Something went wrong when calling function. Error: %s\n", _return_str.c_str());
        else
          printf("[C++] Returned value: %s\n", _return_str.c_str());

        delete _number;
      }

      lua::variant* _res_call = _f_db.call_lua_function("fill_program_state", &_v2);
      if(_res_call && _res_call->get_type() == lua::error_var::get_static_lua_type()){
        lua::error_var* _err_call = (error_var*)_res_call;
        std::string _err_str = _err_call->to_string();
        printf("[C++] Error: %s\n", _err_str.c_str());

        delete _res_call;
      }

      _print_program_state(_state);
      
      lua_getglobal(_state, "program_state");
      lua::table_var _ps_table(_state, -1);
      lua_pop(_state, 1);

      {
        lua::string_var _k1 = "this is a test";
        lua::string_var _k2 = " appended text\0this is a hidden text";

        std::string _res = (_k1 + _k2).to_string();

        printf("appended word = %s\n", _res);
      }

      lua::table_var _new_table; _new_table.push_to_stack(_state);
      lua_setglobal(_state, "program_state");
      _print_program_state(_state);

      _ps_table.push_to_stack(_state);
      lua_setglobal(_state, "program_state");
      _print_program_state(_state);

      _f_db.call_lua_function("call_c_function");

      /*
      lua_pushglobaltable(_state);
      std::string _global_str = lua::to_string(_state, -1);
      printf("parsing global: %s\n", _global_str.c_str());

      lua::table_var _global_var_test(_state, -1);
      printf("parsing successful!\n");
      */
    }
    catch(std::exception* e){
      printf("[C++] Something went wrong when running lua file. Error Message: %s\n", e->what());

      delete e;
    }

    delete _hook_handler;
  }

  printf("[C++] Closing lua state...\n");
  lua_close(_state);
  printf("[C++] Lua state closed\n");
}