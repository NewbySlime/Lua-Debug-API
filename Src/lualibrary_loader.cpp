#include "luainternal_storage.h"
#include "lualibrary_loader.h"
#include "luaobject_util.h"
#include "luautility.h"
#include "luavariant.h"
#include "luavariant_util.h"
#include "std_logger.h"

#define LUD_LIBRARY_LOADER_VAR_NAME " __clua_library_loader"

using namespace lua;
using namespace lua::internal;
using namespace lua::object;


#ifdef LUA_CODE_EXISTS

library_loader::library_loader(lua_State* state){
  _logger = get_std_logger();

  _this_state = NULL;

  library_loader* _test_obj = get_attached_object(state);
  if(_test_obj){
    _logger->print_error("[library_loader] Cannot initialize object. Reason: lua_State already has library_loader.\n");
    return;
  }

  _this_state = state;
  _set_attached_object(state, this);
}

library_loader::~library_loader(){
  if(!_this_state)
    return;

  _set_attached_object(_this_state, NULL);
}


static const luaL_Reg __dump_reg_data[] = {
  {NULL, NULL}
};

int library_loader::_requiref_cb(lua_State* state){
  library_loader* _this = get_attached_object(state);

  // create dump library
  luaL_newlib(_this->_this_state, __dump_reg_data);
  return 1;
}


void library_loader::_set_attached_object(lua_State* state, library_loader* object){
  require_internal_storage(state); // s+1

  // set object to internal storage
  lua_pushstring(state, LUD_LIBRARY_LOADER_VAR_NAME); // s+1
  lua_pushlightuserdata(state, object); // s+1
  lua_settable(state, -3); // s-2

  // pop internal storage
  lua_pop(state, 1); // s-1
}


library_loader* library_loader::get_attached_object(lua_State* state){
  library_loader* _result = NULL;

  require_internal_storage(state); // s+1

  // get object from internal storage
  lua_pushstring(state, LUD_LIBRARY_LOADER_VAR_NAME); // s+1
  lua_gettable(state, -2); // s-1+1
  if(lua_type(state, -1) == LUA_TLIGHTUSERDATA)
    _result = (library_loader*)lua_touserdata(state, -1);

  // pop table result and internal storage
  lua_pop(state, 2); // s-2
  return _result;
}


bool library_loader::load_library(const char* lib_name, I_object* lib_object){
  if(!_this_state)
    return false;

  I_object* _test_obj = get_library_object(lib_name);
  if(_test_obj == lib_object){
    if(_logger)
      _logger->print_error("[library_loader] Cannot add library with same object.\n");
    
    return false;
  }

  luaL_requiref(_this_state, lib_name, _requiref_cb, true);

  // try to deinit current object
  if(lua_getmetatable(_this_state, -1)){
    lua_pop(_this_state, 1); // pop the fetched metatable first

    lua_newtable(_this_state);
    lua_setmetatable(_this_state, -2);
  }

  push_object_to_table(_this_state, lib_object, -1);

  // pop lib table
  lua_pop(_this_state, 1);

  return true;
}

I_object* library_loader::get_library_object(const char* lib_name) const{
  luaL_requiref(_this_state, lib_name, _requiref_cb, true);
  I_object* _result = get_object_from_table(_this_state, -1);

  // pop lib table
  lua_pop(_this_state, 1);
  return _result;
}


void library_loader::set_logger(I_logger* logger){
  _logger = logger;
}



// MARK: DLL definitions

DLLEXPORT lua::I_library_loader* CPPLUA_CREATE_LIBRARY_LOADER(void* istate){
  return new library_loader((lua_State*)istate);
}

DLLEXPORT void CPPLUA_DELETE_LIBRARY_LOADER(lua::I_library_loader* object){
  delete object;
}

#endif // LUA_CODE_EXISTS