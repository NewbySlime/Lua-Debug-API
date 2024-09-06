#include "luaobject_helper.h"


using namespace lua;
using namespace lua::object;


function_store::function_store(object_destructor_func destructor){
  _deinit_func = destructor;
  set_function_data(NULL);
}


void function_store::set_function_data(const fdata* list){
  _function_list = list;
  _func_count = 0;
  if(!_function_list)
    return;

  bool _do_loop = true;
  while(_do_loop){
    _do_loop = _function_list[_func_count].func != NULL;
    _func_count++;
  }

  _func_count--;
}


void function_store::set_destructor(object_destructor_func destructor){
  _deinit_func = destructor;
}


I_object::object_destructor_func function_store::get_this_destructor() const{
  return _deinit_func;
}


int function_store::get_function_count() const{
  return _func_count;
}

const char* function_store::get_function_name(int idx) const{
  if(idx < 0 || idx >= _func_count)
    return NULL;

  return _function_list[idx].name;
}

I_object::lua_function function_store::get_function(int idx) const{
  if(idx < 0 || idx >= _func_count)
    return NULL;

  return _function_list[idx].func;
}