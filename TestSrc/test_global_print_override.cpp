#include "../Src/lua_includes.h"
#include "../Src/luaglobal_print_override.h"
#include "../Src/luaruntime_handler.h"


using namespace lua;
using namespace lua::global;


int main(){
  lua_State* _state = luaL_newstate();
  luaL_openlibs(_state);

  int _err = luaL_dofile(_state, "test_global_print_override.lua");
  if(_err != LUA_OK){
    printf("Cannot open lua file. Error: %d\n", _err);
    exit(_err);
  }

  runtime_handler* _runtime_handler = new runtime_handler(_state);
  print_override* _print_override = new print_override(_state);

  func_db* _func_db = _runtime_handler->get_function_database();
  _func_db->expose_lua_function<nil_var>("test_func");
  
  _runtime_handler->run_execution([](void* data){
    runtime_handler* _runtime_handler = (runtime_handler*)data;
    variant * _res = _runtime_handler->get_function_database()->call_lua_function("test_func");

    if(_res)
      delete _res;
  }, _runtime_handler);

  while(_runtime_handler->is_currently_executing()){
    bool _abnormal_check = false;
    while(_runtime_handler->is_currently_executing()){
      bool _break = false;

      DWORD _code = WaitForSingleObject(_print_override->get_event_handle(), 100);
      switch(_code){
        break; case WAIT_TIMEOUT:
          continue;

        break; case WAIT_OBJECT_0:
          _break = true;

        break; default:
          _abnormal_check = true;
          _break = true;
      }

      if(_break)
        break;
    }

    if(_abnormal_check){
      printf("An abnormal check flagged!\n");
      break;
    }

    if(!_runtime_handler->is_currently_executing())
      break;

    printf("Lua_stdout: %s", _print_override->read_all().c_str());
  }

  printf("Event check finished.\n");

  delete _print_override;
  delete _runtime_handler;
  lua_close(_state);
}