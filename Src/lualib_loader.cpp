#include "lua_variant.h"
#include "lualib_loader.h"
#include "luaobject_helper.h"
#include "stdlogger.h"

#define LUD_LIB_LOADER_VAR_NAME " __clua_lib_loader"

using namespace lua;
using namespace lua::object;


lib_loader::lib_loader(lua_State* state){
  _logger = get_stdlogger();

  _this_state = NULL;

  lib_loader* _test_obj = get_attached_object(state);
  if(_test_obj){
    _logger->print_error("[lib_loader] Cannot initialize object. Reason: lua_State already has lib_loader.\n");
    return;
  }

  _this_state = state;
  _set_attached_object(state, this);
}

lib_loader::~lib_loader(){
  if(!_this_state)
    return;

  _set_attached_object(_this_state, NULL);
}


static const luaL_Reg __dump_reg_data[] = {
  {NULL, NULL}
};

int lib_loader::_requiref_cb(lua_State* state){
  lib_loader* _this = get_attached_object(state);

  // create dump library
  luaL_newlib(_this->_this_state, __dump_reg_data);
  return 1;
}


void lib_loader::_set_attached_object(lua_State* state, lib_loader* object){
  lightuser_var _lud_var = object;
  _lud_var.setglobal(state, LUD_LIB_LOADER_VAR_NAME);
}


lib_loader* lib_loader::get_attached_object(lua_State* state){
  variant* _lud_var = to_variant_fglobal(state, LUD_LIB_LOADER_VAR_NAME);
  lib_loader* _result =
    (lib_loader*)(_lud_var->get_type() == lightuser_var::get_static_lua_type()?((lightuser_var*)_lud_var)->get_data(): NULL);

  delete _lud_var;
  return _result;
}


bool lib_loader::load_library(const char* lib_name, I_object* lib_object){
  if(!_this_state)
    return false;

  I_object* _test_obj = get_library_object(lib_name);
  if(_test_obj == lib_object){
    if(_logger)
      _logger->print_error("[lib_loader] Cannot add library with same object.\n");
    
    return false;
  }

  luaL_requiref(_this_state, lib_name, _requiref_cb, true);
  
  lua_newtable(_this_state);

  // try to deinit current object
  if(lua_getmetatable(_this_state, -2))
    lua_setmetatable(_this_state, -2);

  // pop the new table to remove it with the metatable (object)
  lua_pop(_this_state, 1);

  push_object_to_table(_this_state, lib_object, -1);

  // pop lib table
  lua_pop(_this_state, 1);

  return true;
}

I_object* lib_loader::get_library_object(const char* lib_name) const{
  luaL_requiref(_this_state, lib_name, _requiref_cb, true);
  I_object* _result = get_obj_from_table(_this_state, -1);

  // pop lib table
  lua_pop(_this_state, 1);
  return _result;
}


void lib_loader::set_logger(I_logger* logger){
  _logger = logger;
}