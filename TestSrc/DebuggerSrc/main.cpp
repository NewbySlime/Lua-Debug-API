#include "../../Src/lua_includes.h"

#include "../../Src/luaruntime_handler.h"

#include "conio.h"


#define ESCAPE_CHAR "\x1B"


using namespace lua;
using namespace lua::debug;


void _test_cb(void* data){
  runtime_handler* _handler = (runtime_handler*)data;

  number_var _arg1(20);
  variant* _result = _handler->get_function_database()->call_lua_function("test_loop", &_arg1);
  if(_result)
    delete _result;
}


void _erase_lines_from_bottom(int count){
  printf("\r");
  for(int i = 0; i < count; i++){
    printf(ESCAPE_CHAR "[K");

    if(i < (count-1))
      printf(ESCAPE_CHAR "[1A");
  }
}


char _get_char(){
  printf("Step function:\n\t'1' to step by line\n\t'2' to step in\n\t'3' to step out\n\t'4' to step over\n\t'0' to continue execution\n\t'q' to quit");

  char _res = _getch();
  _erase_lines_from_bottom(7);

  return _res;
}


int main(){
  const char* _filepath = "./test_loop.lua";

  runtime_handler _handler(_filepath);
  execution_flow* _exec_flow = _handler.get_execution_flow();
  func_db* _f_db = _handler.get_function_database();
  _f_db->expose_lua_function<nil_var, number_var>("test_loop");

  _exec_flow->block_execution();
  _handler.run_execution(
    _test_cb,
    &_handler
  );
  
  // wait until pausing
  while(!_exec_flow->currently_pausing() && _handler.is_currently_executing())
    Sleep(10);

  _exec_flow->set_function_name(0, "test_loop");

  while(true){
    while(!_exec_flow->currently_pausing() && _handler.is_currently_executing())
      Sleep(10);

    if(!_handler.is_currently_executing())
      break;
    
    const lua_Debug* _dbg_data = _exec_flow->get_debug_data();
    printf("Current line %d\n", _dbg_data->currentline);
    printf("Function name %s\n", _exec_flow->get_function_name());
    char _key = _get_char();
    _erase_lines_from_bottom(3);

    bool _break_loop = false;
    switch(_key){
      break; case '1':{
        _exec_flow->step_execution(execution_flow::st_per_line);
      }

      break; case '2':{
        _exec_flow->step_execution(execution_flow::st_in);
      }

      break; case '3':{
        _exec_flow->step_execution(execution_flow::st_out);
      }

      break; case '4':{
        _exec_flow->step_execution(execution_flow::st_over);
      }

      break; case '0':{
        _exec_flow->resume_execution();
      }

      break; case 'q':{
        _break_loop = true;
      }
    }

    Sleep(100);

    if(_break_loop)
      break;
  }
}