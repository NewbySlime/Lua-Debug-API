#include "../../src/luaapi_core.h"
#include "../../src/luaapi_memory_util.h"
#include "../../src/luaapi_runtime.h"
#include "../../src/luaapi_thread.h"
#include "../../src/luaapi_variant_util.h"
#include "../../src/luamemory_util.h"
#include "../../src/luathread_control.h"
#include "../../src/luathread_util.h"
#include "../../src/luautility.h"
#include "../../src/luavalue_ref.h"
#include "../../src/luavariant_arr.h"
#include "../memtracker/memtracker.h"
#include "error_util.h"
#include "file_logger.h"
#include "print_override_listener.h"
#include "std_logger.h"
#include "Windows.h"

using namespace dynamic_library::util;
using namespace error::util;
using namespace lua;
using namespace lua::api;
using namespace lua::global;
using namespace lua::memory;
using namespace lua::utility;


#define UNIT_TEST_API_FILE "CPPAPI.dll"
#define UNIT_TEST_MEMTRACKER_API_FILE "memtracker.dll"
#define UNIT_TEST1_LUA_FILE "TestSrc/unit_test1.lua" // test1 contains main test cod
#define UNIT_TEST2_LUA_FILE "TestSrc/unit_test2.lua" // test2 contains code that can be imported
#define UNIT_TEST_LOG_FILE ".log"

#define CHECKING_MATCHING_ERROR(cmp1, cmp2) (cmp1 == cmp2? "\x1B[38;5;46mMATCH\x1B[0m": "\x1B[38;5;196mNOT MATCH\x1B[0m")
#define CHECKING_BOOL(b) (b? "true": "false")

#define VALUE_REF_TEST_REF_KEY "__test_ref"


int main(){
  file_logger* _normal_logger;
  I_logger* _debug_logger = get_std_logger();

  const compilation_context* _cc = NULL;
  // rh1 contains main test code
  I_runtime_handler* _rh1 = NULL;
  // rh2 contains code that can be imported
  I_runtime_handler* _rh2 = NULL;
  I_print_override* _po = NULL;
  print_override_listener* _po_listener = NULL;
  value_ref* _vref1 = NULL;
  value_ref* _vref2 = NULL;
  value_ref* _vref3 = NULL;

  I_thread_handle_reference* _thread_ref = NULL;

  HMODULE _memtracker_module = NULL;
  HMODULE _api_module = NULL;

  I_memtracker* _tracker = NULL;


#define ASSERT_CHECK_WINDOWS_ERROR(condition, message) \
  if((condition)){ \
    std::string _err_msg = get_windows_error_message(GetLastError()); \
    printf("\n%s: %s\n", (message), _err_msg.c_str()); \
    goto on_error; \
  }


  // MARK: Load the library first
  printf("Getting Lua API library... ");
  _api_module = LoadLibraryA(UNIT_TEST_API_FILE);
  ASSERT_CHECK_WINDOWS_ERROR(!_api_module, "Error, Cannot open library");

  printf("Success!\n");

  // MARK: Loading memtracker library
  printf("Loading memtracker library...");
  _memtracker_module = LoadLibraryA(UNIT_TEST_MEMTRACKER_API_FILE);
  ASSERT_CHECK_WINDOWS_ERROR(!_memtracker_module, "Error, Cannot open library");

  memtracker_construct_func _memtracker_constructor = (memtracker_construct_func)GetProcAddress(_memtracker_module, MEMTRACKER_CONSTRUCT_STR);
  ASSERT_CHECK_WINDOWS_ERROR(!_memtracker_constructor, "Error, Cannot open library");
  memtracker_deconstruct_func _memtracker_deconstructor = (memtracker_deconstruct_func)GetProcAddress(_memtracker_module, MEMTRACKER_DECONSTRUCT_STR);
  ASSERT_CHECK_WINDOWS_ERROR(!_memtracker_deconstructor, "Error, Cannot open library");

  _tracker = _memtracker_constructor();
  printf("Success!\n");

{ // enclosure for using goto

  printf("Getting API context... ");

  // MARK: Get API context
  get_api_compilation_context _get_api_context_func = (get_api_compilation_context)GetProcAddress(_api_module, CPPLUA_GET_API_COMPILATION_CONTEXT_STR);
  ASSERT_CHECK_WINDOWS_ERROR(!_get_api_context_func, "Error, Cannot get API context");

  _cc = _get_api_context_func();
  printf("Success!\n");

  // MARK: Set custom memory management function
  _cc->api_memutil->set_memory_manager_config(_tracker->get_configuration());
  set_memory_manager_config(_tracker->get_configuration());
  
  _tracker->check_memory_usage(_debug_logger);

  _normal_logger = new file_logger(UNIT_TEST_LOG_FILE, false);

  printf("Creating runtime_handler... ");

  // MARK: Create runtime handler 1
  _rh1 = _cc->api_runtime->create_runtime_handler(UNIT_TEST1_LUA_FILE, true);
  if(!_rh1->get_lua_state_interface()){
    printf("\n");
    printf("Error: Cannot create runtime_handler 1.\n");
    goto on_error;
  }

  // MARK: Create runtime handler 2
  _rh2 = _cc->api_runtime->create_runtime_handler(UNIT_TEST2_LUA_FILE, true);
  if(!_rh1->get_lua_state_interface()){
    printf("\n");
    printf("Error: Cannot create runtime_handler 2.\n");
    goto on_error;
  }

  printf("Success!\n");

  printf("Loading the file...\n");

  I_thread_control* _rh1_tc = _rh1->get_thread_control_interface();
  printf("Loading runtime_handler 1 ... ");
  _rh1->run_code();
  _thread_ref = _rh1_tc->get_thread_handle(_rh1->get_main_thread_id());
  if(_thread_ref){
    _thread_ref->get_interface()->wait_for_thread_stop();
    _rh1_tc->free_thread_handle(_thread_ref);
  }
  printf("Done!\n");

  I_thread_control* _rh2_tc = _rh2->get_thread_control_interface();
  printf("Loading runtime_handler 2 ... ");
  _rh2->run_code();
  _thread_ref = _rh2_tc->get_thread_handle(_rh2->get_main_thread_id());
  if(_thread_ref){
    _thread_ref->get_interface()->wait_for_thread_stop();
    _rh2_tc->free_thread_handle(_thread_ref);
  }
  printf("Done!\n");

  core _lc = _rh1->get_lua_core_copy();

  printf("Creating print_override... ");

  // MARK: Create print_override
  _po = _cc->api_runtime->create_print_override(_lc.istate);
  if(!_po->get_lua_interface_state()){
    printf("\n");
    printf("Error: Cannot create print_override.\n");
    goto on_error;
  }
  
  printf("Success!\n");

  printf("Creating listener for print_override, ");

  // MARK: Create listener for print_override
  _po_listener = new print_override_listener(_po, _normal_logger, _debug_logger);
  _po_listener->start_listening();

  printf("Success!\n");
  
  printf("Creating dummy coroutine...\n");

  // MARK: Creating dummy coroutine
  core _dummy_lc;
    _dummy_lc.istate = _lc.context->api_value->newthread(_lc.istate);
    _dummy_lc.context = _lc.context;

  bool _chk_same_runtime = check_same_runtime(&_lc, &_dummy_lc);
  printf("\tIs the new coroutine has the same runtime lua_State: %s\n", CHECKING_BOOL(_chk_same_runtime));

  printf("\n");
  printf("Testing value_ref...\n");

  { // MARK: Test value_ref
    const int _magic_number = 256;
    _vref1 = new value_ref(&_lc, VALUE_REF_TEST_REF_KEY, "test_table");
    _vref2 = new value_ref(&_lc, VALUE_REF_TEST_REF_KEY, "test_table");

    printf("Creating Ref #1...\nCreating Ref #2...\n");

    printf("Ref #1, at init it has reference (%s, exp. false), Value is set to table that has magic number...\nRef #1 magic number: %d\n", CHECKING_BOOL(_vref1->reference_initiated()), _magic_number);
    if(_vref1->reference_initiated()){
      printf("Error: reference #1 already initiated? (expected: false).\n");
      goto on_error;
    }

    _lc.context->api_value->newtable(_lc.istate);
    _lc.context->api_value->pushstring(_lc.istate, "magic_number");
    _lc.context->api_value->pushinteger(_lc.istate, _magic_number);
    _lc.context->api_value->settable(_lc.istate, -3);

    _vref1->set_value(-1);
    _lc.context->api_stack->pop(_lc.istate, 1);

    printf("Ref #2, at init it has reference (%s, exp. false).\n", CHECKING_BOOL(_vref2->reference_initiated()));
    if(_vref2->reference_initiated()){
      printf("Error: refrence #2 already initiated? (expected: false).\n");
      goto on_error;
    }

    printf("Updating ref #2... ");
    _vref2->update_reference();
    if(!_vref2->reference_initiated()){
      printf("\n");
      printf("Error: reference #2 updated, yet it does not have the reference?\n");
      goto on_error;
    }

    printf("Success!\n");

    printf("Checking ref #2 value....\n");

    _vref2->push_value_to_stack();
    bool _is_table = _lc.context->api_value->type(_lc.istate, -1) == LUA_TTABLE;
    printf("\tSet value is table: %s (exp. true)\n", CHECKING_BOOL(_is_table));
    if(!_is_table){
      printf("Error: set value is not a table. (should be the same as the set value in ref #1)\n");
      goto on_error;
    }

    _lc.context->api_value->pushstring(_lc.istate, "magic_number");
    _lc.context->api_value->gettable(_lc.istate, -2);
    bool _is_number = _lc.context->api_value->type(_lc.istate, -1) == LUA_TNUMBER;
    printf("\tMagic number value is a type of number: %s (exp. true)\n", CHECKING_BOOL(_is_number));
    if(!_is_number){
      printf("Error: magic number is not a number.\n");
      goto on_error;
    }

    int _result_magic_number = _lc.context->api_value->tointeger(_lc.istate, -1);
    printf("\tMagic number value: %d (exp. %d)\n", _result_magic_number, _magic_number);
    if(_result_magic_number != _magic_number){
      printf("Error: magic number does not match.\n");
      goto on_error;
    }

    _lc.context->api_stack->pop(_lc.istate, 1); // delete the magic number

    printf("\nChanging magic number in ref #2...\n");

    const int _magic_number2 = 64;
    _lc.context->api_value->pushstring(_lc.istate, "magic_number");
    _lc.context->api_value->pushinteger(_lc.istate, _magic_number2);
    _lc.context->api_value->settable(_lc.istate, -3);

    printf("\tSet magic number in ref #2 as: %d\n", _magic_number2);

    _lc.context->api_stack->pop(_lc.istate, 1); // delete the table

    _vref1->push_value_to_stack();
    _lc.context->api_value->pushstring(_lc.istate, "magic_number");
    _lc.context->api_value->gettable(_lc.istate, -2);

    _result_magic_number = _lc.context->api_value->tointeger(_lc.istate, -1);
    printf("\tMagic number value in ref #1: %d (exp. %d)\n", _result_magic_number, _magic_number2);
    if(_result_magic_number != _magic_number2){
      printf("Error: magic number does not match.\n");
      goto on_error;
    }

    _lc.context->api_stack->pop(_lc.istate, 2); // delete result number and the table

    printf("\n");
    printf("Delete ref #1...\n");
    delete _vref1; _vref1 = NULL;

    _vref2->push_value_to_stack();
    bool _ref_still_exists = _lc.context->api_value->type(_lc.istate, -1) != LUA_TNIL;
    printf("Reference #2 still exists: %s (exp. true)\n", CHECKING_BOOL(_ref_still_exists));
    if(!_ref_still_exists){
      printf("Error: reference is deleted? (even only ref #1 being deleted)\n");
      goto on_error;
    }

    printf("Delete ref #2...\n");
    delete _vref2; _vref2 = NULL;

    printf("Creating ref #3...\n");
    _vref3 = new value_ref(&_lc, VALUE_REF_TEST_REF_KEY, "test_table");
    printf("Ref #3, at init has reference %s (exp. false)\n", CHECKING_BOOL(_vref3->reference_initiated()));
    if(_vref3->reference_initiated()){
      printf("Error: reference still exist? (reference might not be deleted after ref #1 and ref #2 deleted)\n");
      goto on_error;
    }

    delete _vref3; _vref3 = NULL;
  }

  {// MARK: Test variants

  printf("\n\nChecking variant...\n");
  
  // MARK: Test number_var
    printf("\nTesting number_var...\n");
    double _num1 = 10.4; number_var _nvar1 = _num1;
    double _num2 = 5.2; number_var _nvar2 = _num2;
    printf("number 1: (%f) | number 2: (%f)\n", _nvar1.get_number(), _nvar2.get_number());
    printf("Testing addition, n1 = n1 + n2\n");
    _num1 = _num1 + _num2; _nvar1 = _nvar1 + _nvar2;
    printf("number 1: (%f, exp. %f) %s\n", _num1, _nvar1.get_number(), CHECKING_MATCHING_ERROR(_nvar1, _num1));
    printf("Testing substraction, n1 = n2 - n1\n");
    _num1 = _num2 - _num1; _nvar1 = _nvar2 - _nvar1;
    printf("number 1: (%f, exp. %f) %s\n", _num1, _nvar1.get_number(), CHECKING_MATCHING_ERROR(_nvar1, _num1));
    printf("Testing multiplication, n1 = n2 * n1\n");
    _num1 = _num2 * _num1; _nvar1 = _nvar2 * _nvar1;
    printf("number 1: (%f, exp. %f) %s\n", _num1, _nvar1.get_number(), CHECKING_MATCHING_ERROR(_nvar1, _num1));
    printf("Testing division, n1 = n1 / n2\n");
    _num1 = _num1 / _num2; _nvar1 = _nvar1 / _nvar2;
    printf("number 1: (%f, exp. %f) %s\n", _num1, _nvar1.get_number(), CHECKING_MATCHING_ERROR(_nvar1, _num1));

    bool _chk_num = _num1 < _num2; bool _chk_var = _nvar1 < _nvar2;
    printf("Testing comparison, n1 < n2, %s (exp. %s), %s\n", CHECKING_BOOL(_chk_var), CHECKING_BOOL(_chk_num), CHECKING_MATCHING_ERROR(_chk_num, _chk_var));
    _chk_num = _num1 > _num2; _chk_var = _nvar1 > _nvar2;
    printf("Testing comparison, n1 > n2, %s (exp. %s), %s\n", CHECKING_BOOL(_chk_var), CHECKING_BOOL(_chk_num), CHECKING_MATCHING_ERROR(_chk_num, _chk_var));

    printf("Pushing to stack...\n");
    _nvar1.push_to_stack(&_lc);
    _num1 = _lc.context->api_value->tonumber(_lc.istate, -1);
    printf("Original number: %f | Pushed number: %f | %s", _nvar1.get_number(), _num1, CHECKING_MATCHING_ERROR(_nvar1, _num1));

    _lc.context->api_stack->pop(_lc.istate, 1); // delete pushed value (number)

    // MARK: Test string_var
    printf("\nTesting string_var... (results compared with std::string)\n");
    std::string _stdstr1 = "test1"; string_var _strvar1 = "test1";
    std::string _stdstr2 = "this_is a test"; string_var _strvar2 = _stdstr2;
    printf("string 1: '%s' | string 2: '%s'\n", _strvar1.get_string(), _strvar2.get_string());
    printf("Testing appending, s1 = s1 + s2\n");
    _stdstr1 = _stdstr1 + _stdstr2; _strvar1 = _strvar1 + _strvar2;
    printf("string 1: ('%s', exp. '%s') %s\n", _stdstr1.c_str(), _strvar1.get_string(), CHECKING_MATCHING_ERROR(_strvar1, _stdstr1));
    
    bool _chk_str = _stdstr1 < _stdstr2; _chk_var = _strvar1 < _strvar2;
    printf("Testing comparison, s1 < s2, %s (exp. %s), %s\n", CHECKING_BOOL(_chk_var), CHECKING_BOOL(_chk_str), CHECKING_MATCHING_ERROR(_chk_str, _chk_var));

    printf("Pushing to stack...\n");
    _strvar1.push_to_stack(&_lc);
    _stdstr1 = _strvar1.get_string();
    _stdstr2 = _lc.context->api_value->tolstring(_lc.istate, -1, NULL);
    printf("Original string: '%s' | Pushed string: '%s' | %s\n", _stdstr1.c_str(), _stdstr2.c_str(), CHECKING_MATCHING_ERROR(_stdstr1, _stdstr2));

    _lc.context->api_stack->pop(_lc.istate, 1); // delete pushed value (string)

    // MARK: Test bool_var
    printf("\nTesting bool_var...\n");
    bool _b1 = true; bool_var _bvar1 = _b1;
    bool _b2 = true; bool_var _bvar2 = _b2;
    printf("bool 1: '%s' | bool 2: '%s'\n", CHECKING_BOOL(_bvar1.get_boolean()), CHECKING_BOOL(_bvar2.get_boolean()));
    
    bool_var _bcopy = _bvar1;
    printf("Testing copying, bool orig. %s, bool copy %s %s\n", CHECKING_BOOL(_bvar1.get_boolean()), CHECKING_BOOL(_bcopy.get_boolean()), CHECKING_MATCHING_ERROR(_bvar1.get_boolean(), _bcopy.get_boolean()));

    bool _chk_bool = _b1 == _b2; _chk_var = _bvar1 == _bvar2;
    printf("Testing comparison, b1 == b2, %s (exp. %s), %s\n", CHECKING_BOOL(_chk_var), CHECKING_BOOL(_chk_bool), CHECKING_MATCHING_ERROR(_chk_var, _chk_bool));
    _chk_bool = _b1 != _b2; _chk_var = _bvar1 != _bvar2;
    printf("Testing comparison, b1 != b2, %s (exp. %s), %s\n", CHECKING_BOOL(_chk_var), CHECKING_BOOL(_chk_bool), CHECKING_MATCHING_ERROR(_chk_var, _chk_bool));

    printf("Pushing to stack...\n");
    _bvar1.push_to_stack(&_lc);
    _b1 = _bvar1.get_boolean();
    _b2 = _lc.context->api_value->toboolean(_lc.istate, -1);
    printf("Original bool: %s | Pushed bool: %s | %s\n", CHECKING_BOOL(_b1), CHECKING_BOOL(_b2), CHECKING_MATCHING_ERROR(_b1, _b2));

    _lc.context->api_stack->pop(_lc.istate, 1); // delete pushed value (bool)
  }

  // MARK: Test runtime_handler
  printf("\nExtended testing for runtime_handler\n");
  printf("Testing stopping infinite loop...\n");
  unsigned long _test_tid = _rh1_tc->run_execution([](void* data){
    I_runtime_handler* _rh1 = (I_runtime_handler*)data;
    core _lc = _rh1->get_lua_core_copy();
    I_thread_handle* _thread = _lc.context->api_thread->get_thread_handle();

    vararr _args;
    vararr _result;

    int* _test = new int();

    const char* _func_name = "infinite_loop_test";
    I_variant* _var = _lc.context->api_varutil->to_variant_fglobal(_lc.istate, _func_name);
    if(_var->get_type() != I_function_var::get_static_lua_type()){
      printf("Error: Function for infinite loop testing is not valid.\n");
      goto skip_execution;
    }

    I_function_var* _fvar = dynamic_cast<I_function_var*>(_var);

    int _errcode = _fvar->run_function(&_lc, &_args, &_result);
    if(_thread->is_stop_signal())
      goto skip_execution;

    if(_errcode != LUA_OK){
      string_store _str;
      if(_result.get_var_count() > 0)
        _result.get_var(0)->to_string(&_str);

      printf("Error: Something went wrong: %s\n", _str.data.c_str());
      goto skip_execution;
    }


    skip_execution:{}
    _lc.context->api_varutil->delete_variant(_var);
    delete _test;
  }, _rh1);

  printf("Delaying 0.5s for good measure...\n");
  Sleep(500);

  _thread_ref = _rh1_tc->get_thread_handle(_test_tid);
  if(!_thread_ref){
    printf("Error: runtime_handler already stopping? It should be infinitely looping...\n");

    _rh1_tc->free_thread_handle(_thread_ref);
    goto on_error;
  }

  _thread_ref->get_interface()->stop_running();
  printf("Infinite loop stopped!\n");

  _rh1_tc->free_thread_handle(_thread_ref);

} // enclosure closing

  on_error:{}

  if(_cc){
    if(_po_listener){
      _po_listener->stop_listening();
      delete _po_listener;
    }

    if(_po)
      _cc->api_runtime->delete_print_override(_po);

    if(_rh1)
      _cc->api_runtime->delete_runtime_handler(_rh1);
    
    if(_rh2)
      _cc->api_runtime->delete_runtime_handler(_rh2);

    if(_vref1)
      delete _vref1;

    if(_vref2)
      delete _vref2;

    if(_vref3)
      delete _vref3;
  }

  if(_api_module)
    FreeLibrary(_api_module);

  if(_normal_logger)
    delete _normal_logger;

  if(_memtracker_module){
    if(_tracker){
      _tracker->check_memory_usage(_debug_logger);
      printf("Custom API allocator: %s\n", _tracker->get_maximal_usage() > 0? "Success!": "Failed.");

      _memtracker_deconstructor(_tracker);
    }

    FreeLibrary(_memtracker_module);
  }
}