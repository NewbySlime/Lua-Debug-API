#include "../../src/luaapi_core.h"
#include "../../src/luaapi_memory_util.h"
#include "../../src/luaapi_runtime.h"
#include "../../src/luaapi_thread.h"
#include "../../src/luaapi_variant_util.h"
#include "../../src/lualibrary_iohandler.h"
#include "../../src/luamemory_util.h"
#include "../../src/luaobject_util.h"
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
using namespace lua::library;
using namespace lua::memory;
using namespace lua::object;
using namespace lua::utility;


#define UNIT_TEST_API_FILE "CPPAPI.dll"
#define UNIT_TEST_MEMTRACKER_API_FILE "memtracker.dll"
#define UNIT_TEST1_LUA_FILE "TestSrc/unit_test1.lua" // test1 contains main test cod
#define UNIT_TEST2_LUA_FILE "TestSrc/unit_test2.lua" // test2 contains code that can be imported
#define UNIT_TEST3_LUA_FILE "TestSrc/unit_test3.lua" // test3 contains code for testing io library
#define UNIT_TEST_LOG_FILE ".log"

#define UNIT_TEST_IO_LIB_LOG_FILE "TestSrc/testiolib.log"

#define CHECKING_MATCHING_ERROR(cmp1, cmp2) (cmp1 == cmp2? "\x1B[38;5;46mMATCH\x1B[0m": "\x1B[38;5;196mNOT MATCH\x1B[0m")
#define CHECKING_ERROR(cmp1, cmp2) (cmp1 == cmp2? "\x1B[38;5;46mOK\x1B[0m": "\x1B[38;5;196mERROR\x1B[0m")
#define CHECKING_BOOL(b) (b? "true": "false")

#define VALUE_REF_TEST_REF_KEY "__test_ref"



class test_table: public function_store, virtual public I_object{
  private:
    std::map<comparison_variant, variant*> _data_list;

    static void _set_error_result(I_vararr* res, long long errcode, const char* msg);
    
  public:
    test_table();
    ~test_table();

    void set_data(const I_variant* key, const I_variant* value);
    I_variant* get_data(const I_variant* key);
    void remove_data(const I_variant* key);

    void free_variant(I_variant* var);

    static void set_data(I_object* object, const I_vararr* parameters, I_vararr* results);
    static void get_data(I_object* object, const I_vararr* parameters, I_vararr* results);
    static void remove_data(I_object* object, const I_vararr* parameters, I_vararr* results);

    lua::I_debuggable_object* as_debug_object() override{return NULL;}

    // top most table is the object representation of the lua object
    void on_object_added(const lua::api::core* lua_core) override{}
};

fdata _test_table_fdata[] = {
  fdata("set_data", test_table::set_data),
  fdata("get_data", test_table::get_data),
  fdata("remove_data", test_table::remove_data),
  fdata(NULL, NULL)
};



int _initiate_table_data(const core* lc);
int _set_magic_number(const core* lc);

int _delay_program(const core* lc);

void _check_function_var(const core* lc, const std::string& test_name, const I_function_var* test_func, const std::string& check_name, const I_function_var* check_func);

void _test_runtime_function(I_runtime_handler* rh);

void _test_table_var_similarities(I_table_var* tv1, I_table_var* tv2);

void __check_stack_pos(const core* lc, int expected_value);


int main(){
  file_logger* _normal_logger = NULL;
  file_logger* _test_io_lib_logger = NULL;
  I_logger* _debug_logger = get_std_logger();

  const compilation_context* _cc = NULL;
  // rh1 contains main test code
  I_runtime_handler* _rh1 = NULL;
  // rh2 contains code that can be imported
  I_runtime_handler* _rh2 = NULL;
  // rh3 contains code for testing io library
  I_runtime_handler* _rh3 = NULL;
  I_print_override* _po_rh1 = NULL;
  I_print_override* _po_rh3 = NULL;
  print_override_listener* _po_rh1_listener = NULL;
  print_override_listener* _po_rh3_listener = NULL;
  value_ref* _vref1 = NULL;
  value_ref* _vref2 = NULL;
  value_ref* _vref3 = NULL;

  I_variant* _get_table_var = NULL;
  I_variant* _change_table_var = NULL;
  I_variant* _get_magic_number_var = NULL;

  I_variant* _test_table_data_var = NULL;
  I_variant* _set_table_data_var = NULL;

  I_thread_handle_reference* _thread_ref = NULL;

  HMODULE _memtracker_module = NULL;
  HMODULE _api_module = NULL;

  I_memtracker* _tracker = NULL;

  // Let it leak, Lua GC will handle it.
  I_io_handler* _rh3_io_lib = NULL;

  function_var* _initiate_table_data_func_var = NULL;
  function_var* _set_magic_number_func_var = NULL;

  function_var* _delay_program_func_var = NULL;

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
  set_memory_manager_config(_tracker->get_configuration());
  printf("Success!\n");

  _initiate_table_data_func_var = new function_var(_initiate_table_data);
  _set_magic_number_func_var = new function_var(_set_magic_number);

  _delay_program_func_var = new function_var(_delay_program);

{ // enclosure for using goto

  printf("Getting API context... ");

  // MARK: Get API context
  get_api_compilation_context _get_api_context_func = (get_api_compilation_context)GetProcAddress(_api_module, CPPLUA_GET_API_COMPILATION_CONTEXT_STR);
  ASSERT_CHECK_WINDOWS_ERROR(!_get_api_context_func, "Error, Cannot get API context");
  create_library_io_handler_func _create_library_io_handler_func = (create_library_io_handler_func)GetProcAddress(_api_module, CPPLUA_LIBRARY_CREATE_IO_HANDLER_STR);

  _cc = _get_api_context_func();
  printf("Success!\n");

  // MARK: Set custom memory management function
  _cc->api_memutil->set_memory_manager_config(_tracker->get_configuration());
  _tracker->check_memory_usage(_debug_logger);

  _normal_logger = new file_logger(UNIT_TEST_LOG_FILE, false);
  _test_io_lib_logger = new file_logger(UNIT_TEST_IO_LIB_LOG_FILE, false);

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

  // MARK: Create runtime handler 3
  _rh3 = _cc->api_runtime->create_runtime_handler(UNIT_TEST3_LUA_FILE, true);

  const core _rh1_lc = _rh1->get_lua_core_copy();
  const core _rh2_lc = _rh2->get_lua_core_copy();
  const core _rh3_lc = _rh3->get_lua_core_copy();
  __check_stack_pos(&_rh1_lc, 0);
  __check_stack_pos(&_rh2_lc, 0);
  __check_stack_pos(&_rh3_lc, 0);
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
  __check_stack_pos(&_rh1_lc, 0);
  printf("Done!\n");

  I_thread_control* _rh2_tc = _rh2->get_thread_control_interface();
  printf("Loading runtime_handler 2 ... ");
  _rh2->run_code();
  _thread_ref = _rh2_tc->get_thread_handle(_rh2->get_main_thread_id());
  if(_thread_ref){
    _thread_ref->get_interface()->wait_for_thread_stop();
    _rh2_tc->free_thread_handle(_thread_ref);
  }
  __check_stack_pos(&_rh2_lc, 0);
  printf("Done!\n");

  I_thread_control* _rh3_tc = _rh3->get_thread_control_interface();
  printf("Loading runtime_handler 3 ... ");
  _rh3->run_code();
  _thread_ref = _rh3_tc->get_thread_handle(_rh3->get_main_thread_id());
  if(_thread_ref){
    _thread_ref->get_interface()->wait_for_thread_stop();
    _rh3_tc->free_thread_handle(_thread_ref);
  }
  _rh3_lc.context->api_varutil->set_global(_rh3_lc.istate, "delay_process", _delay_program_func_var);
  __check_stack_pos(&_rh3_lc, 0);
  printf("Done!\n");

  printf("Creating print_override... ");

  // MARK: Create print_override
  _po_rh1 = _cc->api_runtime->create_print_override(_rh1_lc.istate);
  if(!_po_rh1->get_lua_interface_state()){
    printf("\nError: Cannot create print_override.\n");
    goto on_error;
  }
  __check_stack_pos(&_rh1_lc, 0);

  _po_rh3 = _cc->api_runtime->create_print_override(_rh3_lc.istate);
  if(!_po_rh3->get_lua_interface_state()){
    printf("\nError: Cannot create print_override.\n");
    goto on_error;
  }
  __check_stack_pos(&_rh3_lc, 0);

  printf("Success!\n");

  printf("Creating listener for print_override, ");
  // MARK: Create listener for print_override
  _po_rh1_listener = new print_override_listener(_po_rh1, _normal_logger, _debug_logger);
  _po_rh1_listener->start_listening();
  __check_stack_pos(&_rh1_lc, 0);

  _po_rh3_listener = new print_override_listener(_po_rh3, _test_io_lib_logger, _debug_logger);
  _po_rh3_listener->start_listening();
  __check_stack_pos(&_rh3_lc, 0);
  printf("Success!\n");

  printf("Loading IO Lib for Runtime 3...\n");
  // MARK: Loading IO Lib
  io_handler_api_constructor_data _ioh_constructor_param;
    _ioh_constructor_param.lua_core = &_rh3_lc;
    _ioh_constructor_param.param = NULL;
  _rh3_io_lib = _create_library_io_handler_func(&_ioh_constructor_param);
  _rh3->get_library_loader_interface()->load_library("io", _rh3_io_lib);
  printf("Success!\n");
  
  printf("Creating dummy coroutine...\n");
  // MARK: Creating dummy coroutine
  core _dummy_lc;
    _dummy_lc.istate = _rh1_lc.context->api_value->newthread(_rh1_lc.istate);
    _dummy_lc.context = _rh1_lc.context;

  bool _chk_same_runtime = check_same_runtime(&_rh1_lc, &_dummy_lc);
  printf("\tIs the new coroutine has the same runtime lua_State: %s\n", CHECKING_BOOL(_chk_same_runtime));

  __check_stack_pos(&_rh1_lc, 0);
  printf("\nTesting value_ref...\n");

  { // MARK: Test value_ref
    const int _magic_number = 256;
    _vref1 = new value_ref(&_rh1_lc, VALUE_REF_TEST_REF_KEY, "test_table");
    _vref2 = new value_ref(&_rh1_lc, VALUE_REF_TEST_REF_KEY, "test_table");

    __check_stack_pos(&_rh1_lc, 0);
    printf("Creating Ref #1...\nCreating Ref #2...\n");

    printf("Ref #1, at init it has reference (%s, exp. false), Value is set to table that has magic number...\nRef #1 magic number: %d\n", CHECKING_BOOL(_vref1->reference_initiated()), _magic_number);
    if(_vref1->reference_initiated()){
      printf("Error: reference #1 already initiated? (expected: false).\n");
      goto on_error;
    }

    _rh1_lc.context->api_value->newtable(_rh1_lc.istate);
    _rh1_lc.context->api_value->pushstring(_rh1_lc.istate, "magic_number");
    _rh1_lc.context->api_value->pushinteger(_rh1_lc.istate, _magic_number);
    _rh1_lc.context->api_value->settable(_rh1_lc.istate, -3);

    _vref1->set_value(-1);
    _rh1_lc.context->api_stack->pop(_rh1_lc.istate, 1);

    __check_stack_pos(&_rh1_lc, 0);
    printf("Ref #2, at init it has reference (%s, exp. false).\n", CHECKING_BOOL(_vref2->reference_initiated()));
    if(_vref2->reference_initiated()){
      printf("Error: refrence #2 already initiated? (expected: false).\n");
      goto on_error;
    }

    __check_stack_pos(&_rh1_lc, 0);
    printf("Updating ref #2... ");
    _vref2->update_reference();
    if(!_vref2->reference_initiated()){
      printf("\n");
      printf("Error: reference #2 updated, yet it does not have the reference?\n");
      goto on_error;
    }

    __check_stack_pos(&_rh1_lc, 0);
    printf("Success!\n");

    printf("Checking ref #2 value....\n");
    _vref2->push_value_to_stack();
    bool _is_table = _rh1_lc.context->api_value->type(_rh1_lc.istate, -1) == LUA_TTABLE;
    printf("\tSet value is table: %s (exp. true)\n", CHECKING_BOOL(_is_table));
    if(!_is_table){
      printf("Error: set value is not a table. (should be the same as the set value in ref #1)\n");
      goto on_error;
    }

    _rh1_lc.context->api_value->pushstring(_rh1_lc.istate, "magic_number");
    _rh1_lc.context->api_value->gettable(_rh1_lc.istate, -2);
    bool _is_number = _rh1_lc.context->api_value->type(_rh1_lc.istate, -1) == LUA_TNUMBER;
    printf("\tMagic number value is a type of number: %s (exp. true)\n", CHECKING_BOOL(_is_number));
    if(!_is_number){
      printf("Error: magic number is not a number.\n");
      goto on_error;
    }

    int _result_magic_number = _rh1_lc.context->api_value->tointeger(_rh1_lc.istate, -1);
    printf("\tMagic number value: %d (exp. %d)\n", _result_magic_number, _magic_number);
    if(_result_magic_number != _magic_number){
      printf("Error: magic number does not match.\n");
      goto on_error;
    }

    _rh1_lc.context->api_stack->pop(_rh1_lc.istate, 1); // delete the magic number

    printf("\nChanging magic number in ref #2...\n");
    const int _magic_number2 = 64;
    printf("\tSet magic number in ref #2 as: %d\n", _magic_number2);
    _rh1_lc.context->api_value->pushstring(_rh1_lc.istate, "magic_number");
    _rh1_lc.context->api_value->pushinteger(_rh1_lc.istate, _magic_number2);
    _rh1_lc.context->api_value->settable(_rh1_lc.istate, -3);

    _rh1_lc.context->api_stack->pop(_rh1_lc.istate, 1); // delete the table

    _vref1->push_value_to_stack();
    _rh1_lc.context->api_value->pushstring(_rh1_lc.istate, "magic_number");
    _rh1_lc.context->api_value->gettable(_rh1_lc.istate, -2);

    _result_magic_number = _rh1_lc.context->api_value->tointeger(_rh1_lc.istate, -1);
    printf("\tMagic number value in ref #1: %d (exp. %d)\n", _result_magic_number, _magic_number2);
    if(_result_magic_number != _magic_number2){
      printf("Error: magic number does not match.\n");
      goto on_error;
    }

    _rh1_lc.context->api_stack->pop(_rh1_lc.istate, 2); // delete result number and the table
    __check_stack_pos(&_rh1_lc, 0);

    printf("\n");
    printf("Delete ref #1...\n");
    delete _vref1; _vref1 = NULL;
    __check_stack_pos(&_rh1_lc, 0);

    _vref2->push_value_to_stack();
    bool _ref_still_exists = _rh1_lc.context->api_value->type(_rh1_lc.istate, -1) != LUA_TNIL;
    printf("Reference #2 still exists: %s (exp. true)\n", CHECKING_BOOL(_ref_still_exists));
    if(!_ref_still_exists){
      printf("Error: reference is deleted? (even only ref #1 being deleted)\n");
      goto on_error;
    }

    _rh1_lc.context->api_stack->pop(_rh1_lc.istate, 1);
    __check_stack_pos(&_rh1_lc, 0);

    printf("Delete ref #2...\n");
    delete _vref2; _vref2 = NULL;
    __check_stack_pos(&_rh1_lc, 0);

    printf("Creating ref #3...\n");
    _vref3 = new value_ref(&_rh1_lc, VALUE_REF_TEST_REF_KEY, "test_table");
    __check_stack_pos(&_rh1_lc, 0);
    printf("Ref #3, at init has reference %s (exp. false)\n", CHECKING_BOOL(_vref3->reference_initiated()));
    if(_vref3->reference_initiated()){
      printf("Error: reference still exist? (reference might not be deleted after ref #1 and ref #2 deleted)\n");
      goto on_error;
    }

    delete _vref3; _vref3 = NULL;
    __check_stack_pos(&_rh1_lc, 0);
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
    _nvar1.push_to_stack(&_rh1_lc);
    _num1 = _rh1_lc.context->api_value->tonumber(_rh1_lc.istate, -1);
    printf("Original number: %f | Pushed number: %f | %s", _nvar1.get_number(), _num1, CHECKING_MATCHING_ERROR(_nvar1, _num1));

    _rh1_lc.context->api_stack->pop(_rh1_lc.istate, 1); // delete pushed value (number)
    __check_stack_pos(&_rh1_lc, 0);

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
    _strvar1.push_to_stack(&_rh1_lc);
    _stdstr1 = _strvar1.get_string();
    _stdstr2 = _rh1_lc.context->api_value->tolstring(_rh1_lc.istate, -1, NULL);
    printf("Original string: '%s' | Pushed string: '%s' | %s\n", _stdstr1.c_str(), _stdstr2.c_str(), CHECKING_MATCHING_ERROR(_stdstr1, _stdstr2));

    _rh1_lc.context->api_stack->pop(_rh1_lc.istate, 1); // delete pushed value (string)
    __check_stack_pos(&_rh1_lc, 0);

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
    _bvar1.push_to_stack(&_rh1_lc);
    _b1 = _bvar1.get_boolean();
    _b2 = _rh1_lc.context->api_value->toboolean(_rh1_lc.istate, -1);
    printf("Original bool: %s | Pushed bool: %s | %s\n", CHECKING_BOOL(_b1), CHECKING_BOOL(_b2), CHECKING_MATCHING_ERROR(_b1, _b2));

    _rh1_lc.context->api_stack->pop(_rh1_lc.istate, 1); // delete pushed value (bool)
    __check_stack_pos(&_rh1_lc, 0);

    // MARK: Test function_var
    printf("\nTesting function_var...\n");
    printf("Getting variables from Lua... ");
    _get_table_var = _rh2_lc.context->api_varutil->to_variant_fglobal(_rh2_lc.istate, "get_table");
    if(_get_table_var->get_type() != I_function_var::get_static_lua_type()){
      printf("Error: get_table variable is not a function.\n");
      goto on_error;
    }

    _change_table_var = _rh2_lc.context->api_varutil->to_variant_fglobal(_rh2_lc.istate, "change_table");
    if(_change_table_var->get_type() != I_function_var::get_static_lua_type()){
      printf("Error: change_table variable is not a function.\n");
      goto on_error;
    }

    _get_magic_number_var = _rh2_lc.context->api_varutil->to_variant_fglobal(_rh2_lc.istate, "get_magic_number");
    if(_get_magic_number_var->get_type() != I_function_var::get_static_lua_type()){
      printf("Error: get_magic_number variable is not a function.\n");
      goto on_error;
    }
    __check_stack_pos(&_rh1_lc, 0);

    printf("Success!\n");

    printf("Setting variables from C... ");
    _initiate_table_data_func_var->push_to_stack(&_rh2_lc);
    if(_rh2_lc.context->api_value->type(_rh2_lc.istate, -1) != LUA_TFUNCTION){
      printf("Error: Pushed value from function_var is not a type of function.\n");
      goto on_error;
    }

    if(!_rh2_lc.context->api_value->iscfunction(_rh2_lc.istate, -1)){
      printf("Error: Pushed value from function_var is not a C function?\n");
      goto on_error;
    }

    _rh2_lc.context->api_value->setglobal(_rh2_lc.istate, "initiate_table_data");

    _set_magic_number_func_var->push_to_stack(&_rh2_lc);
    _rh2_lc.context->api_value->setglobal(_rh2_lc.istate, "set_magic_number");
    __check_stack_pos(&_rh1_lc, 0);

    printf("Success!\n");

    printf("Testing filled function_var replaced with another Lua function...\n");
    printf("Testing function_var is a get_table() function.\n");
    function_var _test_func_var(dynamic_cast<I_function_var*>(_get_table_var));
    
    _check_function_var(&_rh2_lc, "Testing function_var", &_test_func_var, "get_table() function", dynamic_cast<I_function_var*>(_get_table_var));

    printf("Testing function_var is replaced with get_magic_number() function.\n");
    _get_magic_number_var->push_to_stack(&_rh2_lc);
    _test_func_var.from_state(&_rh2_lc, -1);
    _rh2_lc.context->api_stack->pop(_rh2_lc.istate, 1);
    
    _check_function_var(&_rh2_lc, "Testing function_var", &_test_func_var, "get_magic_number() function", dynamic_cast<I_function_var*>(_get_magic_number_var));
    __check_stack_pos(&_rh1_lc, 0);

    printf("Success!\n");
    
    printf("\n\nTesting some functions (and also checking the table_var by reference)...\n");
    _test_runtime_function(_rh2);
    __check_stack_pos(&_rh1_lc, 0);
    printf("Done.\n");

    printf("\n\nTesting function_var passed to another lua_State...\n");
    printf("Passing data to another runtime state... ");

    _rh1_lc.context->api_stack->gettop(_rh1_lc.istate);
    _initiate_table_data_func_var->push_to_stack(&_rh1_lc);
    _rh1_lc.context->api_value->setglobal(_rh1_lc.istate, "initiate_table_data");

    _get_table_var->push_to_stack(&_rh1_lc);
    _rh1_lc.context->api_value->setglobal(_rh1_lc.istate, "get_table");

    _get_magic_number_var->push_to_stack(&_rh1_lc);
    _rh1_lc.context->api_value->setglobal(_rh1_lc.istate, "get_magic_number");

    _change_table_var->push_to_stack(&_rh1_lc);
    _rh1_lc.context->api_value->setglobal(_rh1_lc.istate, "change_table");

    _set_magic_number_func_var->push_to_stack(&_rh1_lc);
    _rh1_lc.context->api_value->setglobal(_rh1_lc.istate, "set_magic_number");

    __check_stack_pos(&_rh1_lc, 0);
    printf("Done\n");

    printf("Testing with the same sequence as before...\n");
    _test_runtime_function(_rh1);

    __check_stack_pos(&_rh1_lc, 0);
    printf("Done\n");

    // Test table_var (reference) is in runtime test.
    // MARK: Test table_var (reference, extended)
    printf("Testing table_var (reference, extended)\n");
    printf("Testing passing table data to another state...\n");

    // Note: the functions is in runtime_handler 2
  { // enclosure for scoping
    I_function_var* _get_table_func_var = dynamic_cast<I_function_var*>(_get_table_var);
    vararr _args, _res;
    _initiate_table_data_func_var->run_function(&_rh2_lc, &_args, &_res);

    _args.clear(); _res.clear();
    _get_table_func_var->run_function(&_rh2_lc, &_args, &_res);

    if(_res.get_var_count() <= 0){
      printf("Error: get_table() does not return anything?\n");
      goto on_error;
    }

    const I_variant* _res1 = _res.get_var(0);
    if(_res1->get_type() != I_table_var::get_static_lua_type()){
      printf("Error: get_table() does not return a table?\n");
      goto on_error;
    }

    table_var* _rh2_td = dynamic_cast<table_var*>(cpplua_create_var_copy(_res1));
    _rh2_td->push_to_stack(&_rh1_lc);

    I_table_var* _rh1_td = dynamic_cast<I_table_var*>(_rh1_lc.context->api_varutil->to_variant(_rh1_lc.istate, -1));
    _rh1_lc.context->api_stack->pop(_rh1_lc.istate, 1); // pop pushed table data

    _test_table_var_similarities(_rh2_td, _rh1_td);

    // MARK: Test table_var (copy)
    printf("Testing table_var (copy)\n");
  
    printf("Getting table data from last reference... ");
    table_var _td_copy = _rh2_td;
    _td_copy.as_copy();
    printf("Done.\n");

    string_var _skey1 = "test_data";
    string_var _skey2 = "test number 2";
    number_var _value1 = 3290482390;
    string_var _value2 = "this is a string data";

    _td_copy.set_value(_skey1, &_value1);
  { // enclosure for scoping
    I_variant* _check_value = _td_copy.get_value(_skey1);
    comparison_variant _cmp1 = _value1, _cmp2 = _check_value;
    string_store _keystr, _origstr, _setstr;
      _skey1.to_string(&_keystr);
      _value1.to_string(&_origstr);
      _check_value->to_string(&_setstr);

    printf("Setting key '%s' with value: %s | returned value: %s | %s\n", _keystr.data.c_str(), _origstr.data.c_str(), _setstr.data.c_str(), CHECKING_MATCHING_ERROR(_cmp1, _cmp2));
    _td_copy.free_variant(_check_value);
  } // enclosure closing

    printf("Checking if the previous reference has been modified (next check should has NOT MATCH) or an Error.\n");
    _test_table_var_similarities(_rh2_td, &_td_copy);

    _td_copy.remove_value(_skey1);

  { // enclosure for scoping
    I_variant* _check_value = _td_copy.get_value(_skey1);
    string_store _keystr; _skey1.to_string(&_keystr);
    printf("Removing key '%s', %s\n", _keystr.data.c_str(), CHECKING_ERROR(_check_value, NULL));
    if(_check_value)
      _td_copy.free_variant(_check_value);
  } // enclosure closing

    printf("Creating a similar copy to the table data... ");
    table_var _td_another_copy = _td_copy;
    printf("Done.\n");

    printf("Comparing with previous copy...\n");
    _test_table_var_similarities(&_td_copy, &_td_another_copy);

    printf("Clearing table data, next checking should be Not Match or an Error.\n");
    _td_another_copy.clear_table();
    _test_table_var_similarities(&_td_copy, &_td_another_copy);

    printf("Parsing global value to table_var...\n");
    printf("(NOTE: if an infinite loop happen, there's something wrong.)\n");
    _rh2_lc.context->api_value->pushglobaltable(_rh2_lc.istate);
    table_var _gtvar;
    _gtvar.from_state(&_rh2_lc, -1);
    _gtvar.as_copy();
    _rh2_lc.context->api_stack->pop(_rh2_lc.istate, 1);

    cpplua_delete_variant(_rh2_td);
    _rh1_lc.context->api_varutil->delete_variant(_rh1_td);
  } // enclosure closing

    printf("Testing object_var...\n");
    printf("Creating test table object for Lua...\n");
    test_table* _test_obj = new test_table();
    object_var _test_obj_var(&_rh1_lc, _test_obj);
    _test_obj_var.push_to_stack(&_rh1_lc);
    const void* _test_obj_ref = _rh1_lc.context->api_value->topointer(_rh1_lc.istate, -1);
    _rh1_lc.context->api_stack->pop(_rh1_lc.istate, 1);

#define __UNIT_TEST_MAIN_OBJECT_REF_CHECK(objvar, ptrcheck, msg) \
  { \
    objvar.push_to_stack(&_rh1_lc); \
    const void* _test_ref = _rh1_lc.context->api_value->topointer(_rh1_lc.istate, -1); \
    _rh1_lc.context->api_stack->pop(_rh1_lc.istate, 1); \
    printf("%s | 0x%X | 0x%X | %s\n", msg, (unsigned int)ptrcheck, (unsigned int)_test_ref, CHECKING_MATCHING_ERROR(ptrcheck, _test_ref)); \
  } \
  
  { // enclosure for scoping
    printf("Creating copies and duplicates to check...\n");
    object_var _test_obj_var_copy = _test_obj_var;
    object_var _test_obj_var_construct(&_rh1_lc, _test_obj);
    _test_obj_var.push_to_stack(&_rh1_lc);
    object_var _test_obj_from_state(&_rh1_lc, -1);
    _rh1_lc.context->api_stack->pop(_rh1_lc.istate, 1);

    __UNIT_TEST_MAIN_OBJECT_REF_CHECK(_test_obj_var_copy, _test_obj_ref, "Check by copy")
    __UNIT_TEST_MAIN_OBJECT_REF_CHECK(_test_obj_var_construct, _test_obj_ref, "Check by reconstructing with original object")
    __UNIT_TEST_MAIN_OBJECT_REF_CHECK(_test_obj_from_state, _test_obj_ref, "Check by reading from state")

    printf("Creating object_var using nil values... (Errors are expected)\n");
    _rh1_lc.context->api_value->pushnil(_rh1_lc.istate);
    object_var _test_nil(&_rh1_lc, -1);
    _rh1_lc.context->api_stack->pop(_rh1_lc.istate, 1);
    object_var _test_null(&_rh1_lc, (I_object*)NULL);

    _test_nil.push_to_stack(&_rh1_lc);
    printf("Check by Lua nil %s\n", CHECKING_ERROR(_rh1_lc.context->api_value->type(_rh1_lc.istate, -1), LUA_TNIL));
    _rh1_lc.context->api_stack->pop(_rh1_lc.istate, 1);
    _test_null.push_to_stack(&_rh1_lc);
    printf("Check by CPP NULL %s\n", CHECKING_ERROR(_rh1_lc.context->api_value->type(_rh1_lc.istate, -1), LUA_TNIL));
    _rh1_lc.context->api_stack->pop(_rh1_lc.istate, 1);
  } // enclosure closing

    printf("Passing object_var as an argument of a function...\n");
    _test_table_data_var = _rh1_lc.context->api_varutil->to_variant_fglobal(_rh1_lc.istate, "test_table_data");
    _set_table_data_var = _rh1_lc.context->api_varutil->to_variant_fglobal(_rh1_lc.istate, "set_table_data");

    if(_test_table_data_var->get_type() != I_function_var::get_static_lua_type()){
      printf("Error: test_table_data is not a function?\n");
      goto on_error;
    }

    if(_set_table_data_var->get_type() != I_function_var::get_static_lua_type()){
      printf("Error: set_table_data is not a function?\n");
      goto on_error;
    }

    I_function_var* _test_table_data_fvar = dynamic_cast<I_function_var*>(_test_table_data_var);
    I_function_var* _set_table_data_fvar = dynamic_cast<I_function_var*>(_set_table_data_var);

    string_var _testkey = "test_cpp_key";
    number_var _testvalue = 129038435;
    
    vararr _args;{
      _args.append_var(&_test_obj_var);
      _args.append_var(&_testkey);
      _args.append_var(&_testvalue);
    }
    vararr _res;

    _set_table_data_fvar->run_function(&_rh1_lc, &_args, &_res);

  { // enclosure for scoping
    I_variant* _check_var = _test_obj->get_data(&_testkey);
    string_store _str1, _str2; _testvalue.to_string(&_str1); _check_var->to_string(&_str2);
    comparison_variant _cmp1 = _testvalue, _cmp2 = _check_var;
    printf("Check key '%s' value %s | %s | %s\n", _testkey.get_string(), _str1.data.c_str(), _str2.data.c_str(), CHECKING_MATCHING_ERROR(_cmp1, _cmp2));

    _test_obj->free_variant(_check_var);
  } // enclosure closing

    _args.clear();{
      _args.append_var(&_test_obj_var);
    }
    _res.clear();

    printf("Calling function with test object, check the logs of this unit_test.\n");
    _test_table_data_fvar->run_function(&_rh1_lc, &_args, &_res);
  }

  // MARK: Test runtime_handler
  printf("\nExtended testing for runtime_handler\n");
  printf("Testing stopping an infinite loop...\n");
  unsigned long _test_tid = _rh1_tc->run_execution([](void* data){
    I_runtime_handler* _rh1 = (I_runtime_handler*)data;
    core _lc = _rh1->get_lua_core_copy();
    I_thread_handle* _thread = _lc.context->api_thread->get_thread_handle();

    vararr _args;
    vararr _result;

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
  }, _rh1);
  __check_stack_pos(&_rh1_lc, 0);

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
  __check_stack_pos(&_rh1_lc, 0);

  // MARK: Test io_handler library
  printf("\nTesting IO Library...\n");
  printf("Logs will be redirected to '" UNIT_TEST3_LUA_FILE "' file.\n");
  _test_tid = _rh3_tc->run_execution([](void* data){
    I_runtime_handler* _rh3 = (I_runtime_handler*)data;
    core _lc = _rh3->get_lua_core_copy();
    I_thread_handle* _thread = _lc.context->api_thread->get_thread_handle();

    vararr _args;{
      string_var _logger_file_name = UNIT_TEST_IO_LIB_LOG_FILE; _args.append_var(&_logger_file_name);
    }
    vararr _result;

    const char* _func_name = "test_io_library";
    I_variant* _var = _lc.context->api_varutil->to_variant_fglobal(_lc.istate, _func_name);
    if(_var->get_type() != I_function_var::get_static_lua_type()){
      printf("Error: Function for io library testing is not valid.\n");
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
  }, _rh3);
  __check_stack_pos(&_rh3_lc, 0);

  _thread_ref = _rh3_tc->get_thread_handle(_test_tid);
  if(_thread_ref){
    _thread_ref->get_interface()->wait_for_thread_stop();
    _rh1_tc->free_thread_handle(_thread_ref);
  }
  __check_stack_pos(&_rh3_lc, 0);
  printf("Done.\n");

} // enclosure closing

  on_error:{}

  // MARK: Clearing objects
  if(_cc){
    if(_po_rh1_listener){
      _po_rh1_listener->stop_listening();
      delete _po_rh1_listener;
    }

    if(_po_rh3_listener){
      _po_rh3_listener->stop_listening();
      delete _po_rh3_listener;
    }

    if(_po_rh1)
      _cc->api_runtime->delete_print_override(_po_rh1);
    
    if(_po_rh3)
      _cc->api_runtime->delete_print_override(_po_rh3);

    if(_get_table_var)
      _cc->api_varutil->delete_variant(_get_table_var);

    if(_change_table_var)
      _cc->api_varutil->delete_variant(_change_table_var);

    if(_get_magic_number_var)
      _cc->api_varutil->delete_variant(_get_magic_number_var);

    if(_initiate_table_data_func_var)
      delete _initiate_table_data_func_var;

    if(_set_magic_number_func_var)
      delete _set_magic_number_func_var;

    if(_delay_program_func_var)
      delete _delay_program_func_var;

    if(_test_table_data_var)
      _cc->api_varutil->delete_variant(_test_table_data_var);

    if(_set_table_data_var)
      _cc->api_varutil->delete_variant(_set_table_data_var);

    if(_vref1)
      delete _vref1;

    if(_vref2)
      delete _vref2;

    if(_vref3)
      delete _vref3;

    if(_rh1)
      _cc->api_runtime->delete_runtime_handler(_rh1);
    
    if(_rh2)
      _cc->api_runtime->delete_runtime_handler(_rh2);

    if(_rh3)
      _cc->api_runtime->delete_runtime_handler(_rh3);
  }

  if(_api_module)
    FreeLibrary(_api_module);

  if(_normal_logger)
    delete _normal_logger;

  if(_test_io_lib_logger)
    delete _test_io_lib_logger;

  if(_memtracker_module){
    if(_tracker){
      _tracker->check_memory_usage(_debug_logger);
      printf("Custom API allocator: %s\n", _tracker->get_maximal_usage() > 0? "Success!": "Failed.");
      reset_memory_manager_config();

      _memtracker_deconstructor(_tracker);
    }

    FreeLibrary(_memtracker_module);
  }
}


// MARK: test_table definition

void _default_test_table_destructor(I_object* obj){
  delete obj;
}

test_table::test_table(): function_store(_default_test_table_destructor){
  set_function_data(_test_table_fdata);
}

test_table::~test_table(){
  for(auto _iter: _data_list)
    cpplua_delete_variant(_iter.second);
  _data_list.clear();
}


void test_table::_set_error_result(I_vararr* res, long long errcode, const char* msg){
  res->clear();
  
  string_var _errmsg = msg;
  error_var _errobj(&_errmsg, errcode);
  res->append_var(&_errobj);
}


void test_table::set_data(const I_variant* key, const I_variant* value){
  auto _iter = _data_list.find(key);
  if(_iter != _data_list.end())
    cpplua_delete_variant(_iter->second);

  _data_list[key] = cpplua_create_var_copy(value);
}

I_variant* test_table::get_data(const I_variant* key){
  auto _iter = _data_list.find(key);
  if(_iter == _data_list.end())
    return new nil_var();
  
  return cpplua_create_var_copy(_iter->second);
}

void test_table::remove_data(const I_variant* key){
  auto _iter = _data_list.find(key);
  if(_iter == _data_list.end())
    return;

  _data_list.erase(_iter);
  cpplua_delete_variant(_iter->second);
}


void test_table::free_variant(I_variant* var){
  cpplua_delete_variant(var);
}


void test_table::set_data(I_object* object, const I_vararr* parameters, I_vararr* results){
  test_table* _obj = dynamic_cast<test_table*>(object);
  if(parameters->get_var_count() < 2){
    _set_error_result(results, -1, "Parameter is not enough.");
    return;
  }

  const I_variant* _key = parameters->get_var(0);
  const I_variant* _value = parameters->get_var(1);
  
  _obj->set_data(_key, _value);
}

void test_table::get_data(I_object* object, const I_vararr* parameters, I_vararr* results){
  test_table* _obj = dynamic_cast<test_table*>(object);
  if(parameters->get_var_count() < 1){
    _set_error_result(results, -1, "Parameter is not enough.");
    return;
  }

  const I_variant* _key = parameters->get_var(0);
  I_variant* _res = _obj->get_data(_key);

  results->append_var(_res);
  _obj->free_variant(_res);
}

void test_table::remove_data(I_object* object, const I_vararr* parameters, I_vararr* results){
  test_table* _obj = dynamic_cast<test_table*>(object);
  if(parameters->get_var_count() < 1){
    _set_error_result(results, -1, "Parameter is not enough.");
    return;
  }

  const I_variant* _key = parameters->get_var(0);
  _obj->remove_data(_key);
}



// MARK: misc function definition

int _initiate_table_data(const core* lc){
  int _rtype = lc->context->api_value->getglobal(lc->istate, "table_data");
  if(_rtype != LUA_TTABLE){
    lc->context->api_stack->pop(lc->istate, 1);

    lc->context->api_value->newtable(lc->istate);
    lc->context->api_value->setglobal(lc->istate, "table_data");

    return _initiate_table_data(lc);
  }

  lc->context->api_value->pushstring(lc->istate, "test_string");
  lc->context->api_value->pushstring(lc->istate, "this is a test");
  lc->context->api_value->settable(lc->istate, -3);

  lc->context->api_value->pushstring(lc->istate, "test_number");
  lc->context->api_value->pushnumber(lc->istate, 1028);
  lc->context->api_value->settable(lc->istate, -3);

  lc->context->api_value->pushstring(lc->istate, "test_boolean");
  lc->context->api_value->pushboolean(lc->istate, false);
  lc->context->api_value->settable(lc->istate, -3);

  lc->context->api_stack->pop(lc->istate, 1);
  return 0;
}

int _set_magic_number(const core* lc){
  if(lc->context->api_stack->gettop(lc->istate) <= 0){
    printf("[set_magic_number func] Argument is insufficient.\n");
    return 0;
  }

  if(lc->context->api_value->type(lc->istate, 1) != LUA_TNUMBER){
    printf("[set_magic_number func] Argument is not a number.\n");
    return 0;
  }

  lc->context->api_stack->pushvalue(lc->istate, 1);
  lc->context->api_value->setglobal(lc->istate, "magic_number");

  return 0;
}


int _delay_program(const core* lc){
  if(lc->context->api_stack->gettop(lc->istate) <= 0){
    printf("[delay_program func] Argument is insufficient.\n");
    return 0;
  }

  if(lc->context->api_value->type(lc->istate, 1) != LUA_TNUMBER){
    printf("[delay_program func] Argument is not a number.\n");
    return 0;
  }

  I_number_var* _time_var = dynamic_cast<I_number_var*>(lc->context->api_varutil->to_variant(lc->istate, 1));
  Sleep((DWORD)_time_var->get_number());
  lc->context->api_varutil->delete_variant(_time_var);

  return 0;
}


// functions should be a getter function
void _check_function_var(const core* lc, const std::string& test_name, const I_function_var* test_func, const std::string& check_name, const I_function_var* check_func){
  vararr _test_args, _test_res;
  vararr _check_args, _check_res;

  int _test_err = test_func->run_function(lc, &_test_args, &_test_res);
  int _check_err = check_func->run_function(lc, &_check_args, &_check_res);
  printf("Result error codes; %s: %d | %s: %d | %s\n", test_name.c_str(), _test_err, check_name.c_str(), _check_err, CHECKING_MATCHING_ERROR(_test_err, _check_err));
  printf("Result lengths; %s: %d | %s: %d | %s\n", test_name.c_str(), _test_res.get_var_count(), check_name.c_str(), _check_res.get_var_count(), CHECKING_MATCHING_ERROR(_test_res.get_var_count(), _check_res.get_var_count()));
  if(_test_res.get_var_count() != _check_res.get_var_count()){
    printf("Error: Results count different, skipping to check the results.\n");
    return;
  }

  for(int i = 0; i < _test_res.get_var_count(); i++){
    comparison_variant _cmp_test = _test_res.get_var(i);
    comparison_variant _cmp_check = _check_res.get_var(i);

    string_store _str_test; _cmp_test.get_comparison_data()->to_string(&_str_test);
    string_store _str_check; _cmp_check.get_comparison_data()->to_string(&_str_check);

    printf("%d. Result %s: %s | %s: %s | %s\n", i+1, test_name.c_str(), _str_test.data.c_str(), check_name.c_str(), _str_check.data.c_str(), CHECKING_MATCHING_ERROR(_cmp_test, _cmp_check));
  }
}


void _test_runtime_function(I_runtime_handler* rh){
  const core _lc = rh->get_lua_core_copy();
  I_variant* _initiate_table_data_var = _lc.context->api_varutil->to_variant_fglobal(_lc.istate, "initiate_table_data");
  I_variant* _get_table_var = _lc.context->api_varutil->to_variant_fglobal(_lc.istate, "get_table");
  I_variant* _get_magic_number_var = _lc.context->api_varutil->to_variant_fglobal(_lc.istate, "get_magic_number");
  I_variant* _change_table_var = _lc.context->api_varutil->to_variant_fglobal(_lc.istate, "change_table");
  I_variant* _set_magic_number_var = _lc.context->api_varutil->to_variant_fglobal(_lc.istate, "set_magic_number");

  table_var* _table_data = NULL;

  vararr _args, _res;

#define __TEST_RUNTIME_FUNCTION_CHECK_FUNCVAR(var, name) \
  if(var->get_type() != I_function_var::get_static_lua_type()){ \
    printf("Error: %s variable is not a function.\n", name); \
    goto skip_to_return; \
  } \


  __TEST_RUNTIME_FUNCTION_CHECK_FUNCVAR(_initiate_table_data_var, "initiate_table_data");
  __TEST_RUNTIME_FUNCTION_CHECK_FUNCVAR(_get_table_var, "get_table")
  __TEST_RUNTIME_FUNCTION_CHECK_FUNCVAR(_get_magic_number_var, "get_magic_number");
  __TEST_RUNTIME_FUNCTION_CHECK_FUNCVAR(_change_table_var, "change_table");
  __TEST_RUNTIME_FUNCTION_CHECK_FUNCVAR(_set_magic_number_var, "set_magic_number");

#define __TEST_RUNTIME_FUNCTION_RESET_VARARR() _args.clear(); _res.clear()

{ // enclosure for using gotos
  I_function_var* _initiate_table_data_fvar = dynamic_cast<I_function_var*>(_initiate_table_data_var);
  I_function_var* _get_table_fvar = dynamic_cast<I_function_var*>(_get_table_var);
  I_function_var* _get_magic_number_fvar = dynamic_cast<I_function_var*>(_get_magic_number_var);
  I_function_var* _change_table_fvar = dynamic_cast<I_function_var*>(_change_table_var);
  I_function_var* _set_magic_number_fvar = dynamic_cast<I_function_var*>(_set_magic_number_var);

  const int _magic_number1 = 2048;
  printf("Testing change magic number (new number: %d)... ", _magic_number1);
  __TEST_RUNTIME_FUNCTION_RESET_VARARR();{
    number_var _arg1 = _magic_number1; _args.append_var(&_arg1);
  }
  _set_magic_number_fvar->run_function(&_lc, &_args, &_res);

  __TEST_RUNTIME_FUNCTION_RESET_VARARR();
  _get_magic_number_fvar->run_function(&_lc, &_args, &_res);
  if(_res.get_var_count() <= 0){
    printf("Error: Result value is nil.\n");
    goto skip_to_return;
  }

{ // enclosure for scoping
  const I_variant* _res1 = _res.get_var(0);
  if(_res1->get_type() != I_number_var::get_static_lua_type()){
    printf("Error: Result value is not a number?\n");
    goto skip_to_return;
  }
  
  const I_number_var* _test_number_var = dynamic_cast<const I_number_var*>(_res1);
  int _test_number = _test_number_var->get_number();
  printf("Getter: %d %s\n", _test_number, CHECKING_MATCHING_ERROR(_magic_number1, _test_number));
} // enclosure closing

  const int _magic_number2 = 9012;
  printf("Testing change magic number (new number: %d)... ", _magic_number2);
  __TEST_RUNTIME_FUNCTION_RESET_VARARR();{
    number_var _arg1 = _magic_number2; _args.append_var(&_arg1);
  }
  _set_magic_number_fvar->run_function(&_lc, &_args, &_res);

  __TEST_RUNTIME_FUNCTION_RESET_VARARR();
  _get_magic_number_fvar->run_function(&_lc, &_args, &_res);
  if(_res.get_var_count() <= 0){
    printf("Error: Result value is nil.\n");
    goto skip_to_return;
  }

{ // enclosure for scoping
  const I_variant* _res1 = _res.get_var(0);
  if(_res1->get_type() != I_number_var::get_static_lua_type()){
    printf("Error: Result value is not a number?\n");
    goto skip_to_return;
  }

  const I_number_var* _test_number_var = dynamic_cast<const I_number_var*>(_res1);
  int _test_number = _test_number_var->get_number();
  printf("Getter: %d %s\n", _test_number, CHECKING_MATCHING_ERROR(_magic_number2, _test_number));
} // enclosure closing

  printf("Testing table_var (reference)...\n");
  printf("Initializing table data... ");
  __TEST_RUNTIME_FUNCTION_RESET_VARARR();
  _initiate_table_data_fvar->run_function(&_lc, &_args, &_res);
  printf("Done.\n");

  printf("Getting table data (s Lua, g CPP).\n");
  __TEST_RUNTIME_FUNCTION_RESET_VARARR();
  _get_table_fvar->run_function(&_lc, &_args, &_res);
  if(_res.get_var_count() <= 0){
    printf("Error: Result value is nil.\n");
    goto skip_to_return;
  }

{ // enclosure for scoping
  const I_variant* _res1 = _res.get_var(0);
  if(_res1->get_type() != I_table_var::get_static_lua_type()){
    printf("Error: Result value is not a table?\n");
    goto skip_to_return;
  }

  string_var _skey1 = "test_key";
  string_var _skey2 = "test_key_number_2";
  number_var _value1 = 33248;
  string_var _value2 = "test_value";
  bool_var _value3 = false;

  _table_data = dynamic_cast<table_var*>(cpplua_create_var_copy(_res1));
  _table_data->push_to_stack(&_lc);

  _skey1.push_to_stack(&_lc);
  _value1.push_to_stack(&_lc);
  _lc.context->api_value->settable(_lc.istate, -3);
  
{ // enclosure for scoping
  I_variant* _check_value = _table_data->get_value(&_skey1);
  string_store _str1, _str2; _value1.to_string(&_str1); _check_value->to_string(&_str2);
  comparison_variant _cmp1 = _value1, _cmp2 = _check_value;
  printf("Check key: '%s', %s | %s | %s\n", _skey1.get_string(), _str1.data.c_str(), _str2.data.c_str(), CHECKING_MATCHING_ERROR(_cmp1, _cmp2));

  _table_data->free_variant(_check_value);
} // enclosure closing

  printf("Replacing the value, for another test.\n");

  _skey1.push_to_stack(&_lc);
  _value2.push_to_stack(&_lc);
  _lc.context->api_value->settable(_lc.istate, -3);

{ // enclosure for scoping
  I_variant* _check_value = _table_data->get_value(&_skey1);
  string_store _str1, _str2; _value2.to_string(&_str1); _check_value->to_string(&_str2);
  comparison_variant _cmp1 = _value2, _cmp2 = _check_value;
  printf("Check key: '%s', %s | %s | %s\n", _skey1.get_string(), _str1.data.c_str(), _str2.data.c_str(), CHECKING_MATCHING_ERROR(_cmp1, _cmp2));

  _table_data->free_variant(_check_value);
} // enclosure closing

  printf("Setting table data (g Lua, s CPP)\n");
  _table_data->set_value(_skey1, &_value3);

  _skey1.push_to_stack(&_lc);
  _lc.context->api_value->gettable(_lc.istate, -2);

{ // enclosure for scoping
  I_variant* _check_value = _lc.context->api_varutil->to_variant(_lc.istate, -1);
  string_store _str1, _str2; _value3.to_string(&_str1); _check_value->to_string(&_str2);
  comparison_variant _cmp1 = _value3, _cmp2 = _check_value;
  printf("Check key: '%s', %s | %s | %s\n", _skey1.get_string(), _str1.data.c_str(), _str2.data.c_str(), CHECKING_MATCHING_ERROR(_cmp1, _cmp2));

  _lc.context->api_varutil->delete_variant(_check_value);
} // enclosure closing;

  _lc.context->api_stack->pop(_lc.istate, 1); // remove _skey1

  printf("Removing table data value (key: '%s')\n", _skey1.get_string());
  _table_data->remove_value(&_skey1);

{ // enclosure for scoping
  _skey1.push_to_stack(&_lc);
  int _check_type = _lc.context->api_value->gettable(_lc.istate, -2);
  I_variant* _check_value = _table_data->get_value(&_skey1);
  printf("Value removed (Lua check) %s\n", CHECKING_ERROR(_check_type, LUA_TNIL));
  printf("Value removed (CPP check) %s\n", CHECKING_ERROR(_check_value, NULL));

  if(_check_value)
    _table_data->free_variant(_check_value);
  _lc.context->api_stack->pop(_lc.istate, 1); // remove table result

} // enclosure closing

  printf("Duplicating reference object with another table_var\n");
  
{ // enclosure for scoping
  table_var _test_table_data = _table_data;
  printf("Setting '%s' key of dupe with a value...\n", _skey1.get_string());
  _test_table_data.set_value(_skey1, &_value1);
  printf("Setting '%s' key of dupe with a value...\n", _skey2.get_string());
  _test_table_data.set_value(_skey2, &_value2);

  printf("Getting '%s' key from original table value...\n", _skey1.get_string());
{ // enclosure for scoping
  I_variant* _check_value = _table_data->get_value(&_skey1);
  string_store _str1, _str2; _value1.to_string(&_str1); _check_value->to_string(&_str2);
  comparison_variant _cmp1 = _value1, _cmp2 = _check_value;

  printf("Check key: '%s', %s | %s | %s\n", _skey1.get_string(), _str1.data.c_str(), _str2.data.c_str(), CHECKING_MATCHING_ERROR(_cmp1, _cmp2));

  _table_data->free_variant(_check_value);
} // enclosure closing

  printf("Clearing table data from dupe...\n");
  _test_table_data.clear_table();

  I_variant* _check_value1 = _table_data->get_value(&_skey1);
  I_variant* _check_value2 = _table_data->get_value(&_skey2);
  printf("Value of key '%s' removed, %s\n", _skey1.get_string(), CHECKING_ERROR(_check_value1, NULL));
  printf("Value of key '%s' removed, %s\n", _skey2.get_string(), CHECKING_ERROR(_check_value2, NULL));

  if(_check_value1)
    _table_data->free_variant(_check_value1);
  if(_check_value2)
    _table_data->free_variant(_check_value2);
} // enclosure closing

  _lc.context->api_stack->pop(_lc.istate, 1); // remove _table_data

} // enclosure closing

} // enclosure closing

  skip_to_return:{}

  if(_table_data) 
    cpplua_delete_variant(_table_data);

  _lc.context->api_varutil->delete_variant(_initiate_table_data_var);
  _lc.context->api_varutil->delete_variant(_get_table_var);
  _lc.context->api_varutil->delete_variant(_get_magic_number_var);
  _lc.context->api_varutil->delete_variant(_change_table_var);
  _lc.context->api_varutil->delete_variant(_set_magic_number_var);
}


void _test_table_var_similarities(I_table_var* tv1, I_table_var* tv2){
  std::set<comparison_variant> _key_list;

  const I_variant** _key_data = tv1->get_keys();
  int _idx = 0;
  while(_key_data[_idx]){
    _key_list.insert(_key_data[_idx]);
    _idx++;
  }

  _key_data = tv2->get_keys();
  _idx = 0;
  while(_key_data[_idx]){
    const I_variant* _keyval = _key_data[_idx];
    string_store _keystr; _keyval->to_string(&_keystr);

    auto _iter = _key_list.find(_keyval);
    if(_iter == _key_list.end()){
      printf("Error: key '%s' difference in another table.\n", _keystr.data.c_str());
      return;
    }

    _idx++;
  }

  if(_key_list.size() != _idx){
    printf("Error: key count difference in another table.\n");
    return;
  }

  _idx = 0;
  for(comparison_variant _cmp: _key_list){
    I_variant* _val1 = tv1->get_value(_cmp.get_comparison_data());
    I_variant* _val2 = tv2->get_value(_cmp.get_comparison_data());
    
    comparison_variant _cmp1 = _val1, _cmp2 = _val2;
    string_store _keystr; _cmp.get_comparison_data()->to_string(&_keystr);
    string_store _str1, _str2; _val1->to_string(&_str1); _val2->to_string(&_str2);

    printf("%d. Check key '%s': %s | %s | %s\n", _idx+1, _keystr.data.c_str(), _str1.data.c_str(), _str2.data.c_str(), CHECKING_MATCHING_ERROR(_cmp1, _cmp2));

    tv1->free_variant(_val1);
    tv2->free_variant(_val2);

    _idx++;
  }
}


void __check_stack_pos(const core* lc, int expected_value){
  int _top = lc->context->api_stack->gettop(lc->istate);
  if(_top != expected_value)
    printf("STACK INSTABILITY ERROR: stack length does not match with expected length (current %d | expected %d)\n", _top, expected_value);
}