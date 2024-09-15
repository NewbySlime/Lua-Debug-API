#include "../src/lua_includes.h"
#include "../src/lua_variant.h"
#include "../src/luafunction_database.h"
#include "../src/luaobject_helper.h"
#include "../src/luaruntime_handler.h"
#include "../src/lualib_loader.h"


using namespace lua;
using namespace lua::object;



class test_object: public function_store, virtual public I_object{
  private:
    double _current_num = 0;

    static void _destructor(I_object* obj);

  public:
    test_object();

    static void get_current_number(I_object* obj, const I_vararr* params, I_vararr* result);
    static void add_current_number(I_object* obj, const I_vararr* params, I_vararr* result);

    double get_current_number() const;
    void add_current_number(double number);

    I_debuggable_object* as_debug_object() override{return NULL;}
};


const fdata tobject_list[] = {
  fdata("add_current_number", test_object::add_current_number),
  fdata("get_current_number", test_object::get_current_number)
};


void _test_cb(const I_vararr* args, I_vararr* results){
  test_object* _test = new test_object();
  object_var _obj = _test;

  results->append_var(&_obj);
}


int main(){
  runtime_handler* _rh = new runtime_handler("TestSrc/test_object.lua", false);
  func_db* _fdb = _rh->get_function_database();
  lib_loader* _lib_loader = _rh->get_library_loader();

  _fdb->expose_c_function_nonstrict("create_test", _test_cb);

  HANDLE _event = CreateEvent(NULL, FALSE, FALSE, NULL);
  _rh->register_event_execution_finished(_event);

  test_object* _object1 = new test_object();
  test_object* _object2 = new test_object();
  _lib_loader->load_library("test_lib", _object1);
  _lib_loader->load_library("test_lib", _object2);
  _lib_loader->load_library("test_lib", _object2);

  _rh->run_execution([](void* data){
    runtime_handler* _rh = (runtime_handler*)data;
    int _err_code = _rh->run_current_file();
    if(_err_code != LUA_OK){
      string_store _str;
      _rh->get_last_error_object()->to_string(&_str);

      printf("ERR: %s\n", _str.data.c_str());
    }
  }, _rh);

  WaitForSingleObject(_event, INFINITE);
  CloseHandle(_event);
  
  delete _rh;
}



// MARK: test_object implementation

test_object::test_object(): function_store(_destructor){
  set_function_data(tobject_list);
}

void test_object::get_current_number(I_object* obj, const I_vararr* params, I_vararr* result){
  number_var _res = dynamic_cast<test_object*>(obj)->get_current_number();
  result->append_var(&_res);
}

void test_object::add_current_number(I_object* obj, const I_vararr* params, I_vararr* result){
  if(params->get_var_count() <= 0){
    string_var _err_msg = "Parameter is not enough.";
    error_var _err_obj(&_err_msg, -1);

    result->append_var(&_err_obj);
    return;
  }

  const I_variant* _num_var = params->get_var(0);
  if(_num_var->get_type() != number_var::get_static_lua_type()){
    string_var _err_msg = "Parameter is invalid.";
    error_var _err_obj(&_err_msg, -1);

    result->append_var(&_err_obj);
    return;
  }

  (dynamic_cast<test_object*>(obj))->add_current_number(dynamic_cast<const I_number_var*>(_num_var)->get_number());
}


void test_object::_destructor(I_object* obj){
  delete obj;
}


double test_object::get_current_number() const{
  return _current_num;
}

void test_object::add_current_number(double number){
  _current_num += number;
}