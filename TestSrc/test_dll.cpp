#include "Windows.h"
#include "../Src/luaruntime_handler.h"
#include "../Src/luaglobal_print_override.h"


using namespace lua;
using namespace lua::global;


void _do_add(const lua::I_vararr* args, lua::I_vararr* results){
  number_var* _v_n1 = (number_var*)cpplua_create_var_copy(args->get_var(0));
  number_var* _v_n2 = (number_var*)cpplua_create_var_copy(args->get_var(1));

  number_var _res1 = *_v_n1 + *_v_n2;
  results->append_var(&_res1);

  cpplua_delete_variant(_v_n1);
  cpplua_delete_variant(_v_n2);
}

void _do_sub(const lua::I_vararr* args, lua::I_vararr* results){
  number_var* _v_n1 = (number_var*)cpplua_create_var_copy(args->get_var(0));
  number_var* _v_n2 = (number_var*)cpplua_create_var_copy(args->get_var(1));

  number_var _res1 = *_v_n1 - *_v_n2;
  results->append_var(&_res1);

  cpplua_delete_variant(_v_n1);
  cpplua_delete_variant(_v_n2);
}

void _do_mult(const lua::I_vararr* args, lua::I_vararr* results){
  number_var* _v_n1 = (number_var*)cpplua_create_var_copy(args->get_var(0));
  number_var* _v_n2 = (number_var*)cpplua_create_var_copy(args->get_var(1));

  number_var _res1 = *_v_n1 * *_v_n2;
  results->append_var(&_res1);

  cpplua_delete_variant(_v_n1);
  cpplua_delete_variant(_v_n2);
}


void _read_print_override(I_runtime_handler* r_handler, I_print_override* p_override){
  while(r_handler->is_currently_executing()){
    bool _abnormal_check = false;
    while(r_handler->is_currently_executing() && !p_override->available_bytes()){
      bool _break = false;

      DWORD _code = WaitForSingleObject(p_override->get_event_handle(), 100);
      printf("test %d\n", _code);
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

    if(!p_override->available_bytes())
      continue;

    string_store _str; p_override->read_all(&_str);
    printf("Lua_stdout: %s", _str.data.c_str());
  }

  printf("Event check finished.\n");
}


int main(){
  HMODULE _lib = LoadLibraryA("./CPPAPI.dll");
  if(!_lib){
    DWORD _err_code = GetLastError();
    printf("Cannot open library. Error: %d\n", _err_code);
    exit(_err_code);
  }

  rh_create_func _rh_create_func = (rh_create_func)GetProcAddress(_lib, CPPLUA_CREATE_RUNTIME_HANDLER_STR);
  rh_delete_func _rh_delete_func = (rh_delete_func)GetProcAddress(_lib, CPPLUA_DELETE_RUNTIME_HANDLER_STR);

  gpo_create_func _gpo_create_func = (gpo_create_func)GetProcAddress(_lib, CPPLUA_CREATE_GLOBAL_PRINT_OVERRIDE_STR);
  gpo_delete_func _gpo_delete_func = (gpo_delete_func)GetProcAddress(_lib, CPPLUA_DELETE_GLOBAL_PRINT_OVERRIDE_STR);
  

  if(!_rh_create_func){
    DWORD _err_code = GetLastError();
    printf("Cannot get function from library. Error: %d\n", _err_code);
    exit(_err_code);
  }


  {// MARK: Error load-time scenario
    printf("Testing error scenario\n");
    printf("Creating runtime_handler...\n");
    I_runtime_handler* _r_handler = _rh_create_func(NULL, false, false);
    I_print_override* _p_override = _gpo_create_func(_r_handler->get_lua_state_interface());

    _r_handler->run_execution([](void* cbdata){
      I_runtime_handler* _r_handler = (I_runtime_handler*)cbdata;
      int _err_code = _r_handler->load_file("./TestSrc/test_dll.lua");
      if(_err_code != LUA_OK){
        string_store _str;
        _r_handler->get_last_error_object()->to_string(&_str);

        printf("Error when running lua. Msg: %s\n", _str.data.c_str());
      }

      _err_code = _r_handler->run_current_file();
      if(_err_code != LUA_OK){
        string_store _str;
        _r_handler->get_last_error_object()->to_string(&_str);

        printf("Error when running lua. Msg: %s\n", _str.data.c_str());
      }
    }, _r_handler);

    _read_print_override(_r_handler, _p_override);
    
    printf("Deleting runtime_handler...\n");
    _gpo_delete_func(_p_override);
    _rh_delete_func(_r_handler);
  }


  {// MARK: Normal scenario
    printf("Testing normal scenario\n");
    printf("Creating runtime_handler...\n");
    I_runtime_handler* _r_handler = _rh_create_func("./TestSrc/test_dll.lua", true, true);
    if(!_r_handler->get_lua_state_interface())
      exit(-1);

    I_print_override* _print_override = _gpo_create_func(_r_handler->get_lua_state_interface());

    I_func_db* _func_db = _r_handler->get_function_database_interface();
    _func_db->expose_c_function_nonstrict("do_add", _do_add);
    _func_db->expose_c_function_nonstrict("do_sub", _do_sub);
    _func_db->expose_c_function_nonstrict("do_mult", _do_mult);

    _r_handler->run_execution([](void* data){
      I_runtime_handler* _r_handler = (I_runtime_handler*)data;
      I_func_db* _func_db = (I_func_db*)_r_handler->get_function_database_interface();

      vararr _arg_list;{
        number_var* _arg1 = new number_var(20); 
        _arg_list.append_var(_arg1);
        delete _arg1;

        number_var* _arg2 = new number_var(7); 
        _arg_list.append_var(_arg2);
        delete _arg2;
      }
      
      vararr _result_list;

      _func_db->call_lua_function_nonstrict("do_calculate", &_arg_list, &_result_list);

      string_store _str;
      const I_number_var* _res1 = dynamic_cast<const I_number_var*>(_result_list.get_var(0));
      _res1->to_string(&_str);

      printf("Returned data %s\n", _str.data.c_str());
    }, _r_handler);

    _read_print_override(_r_handler, _print_override);

    printf("Removing print_override...\n");
    _gpo_delete_func(_print_override);

    printf("Removing runtime_handler...\n");
    _rh_delete_func(_r_handler);
  }

  FreeLibrary(_lib);
  printf("Program quit safely.\n");
}