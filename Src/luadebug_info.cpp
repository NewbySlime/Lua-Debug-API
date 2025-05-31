#include "luadebug_info.h"
#include "luamemory_util.h"


using namespace lua::debug;
using namespace lua::memory;
using namespace ::memory;


static const I_dynamic_management* __dm = get_memory_manager();


function_debug_info::function_debug_info(const I_function_debug_info* obj){
  string_store _str;

  for(int i = 0; i < obj->get_parameter_name_count(); i++){
    obj->get_parameter_name(i, &_str);
    _parameter_name_list.push_back(_str.data);
  }
  
  obj->get_file_path(&_str);
  _file_path = _str.data;
  obj->get_function_name(&_str);
  _function_name = _str.data;
  
  _line_defined = obj->get_line_defined();
  _last_line_defined = obj->get_last_line_defined();
  _is_lua_function = obj->is_lua_function();
}

#ifdef LUA_CODE_EXISTS
function_debug_info::function_debug_info(lua_State* state, int idx){
  lua_pushvalue(state, idx);
  for(int i = 1; ; i++){
    const char* _name = lua_getlocal(state, NULL, i);
    if(!_name)
      break;

    _parameter_name_list.push_back(_name);
  }

  lua_Debug _dbg; lua_getinfo(state, ">nS", &_dbg);

  if(std::string(_dbg.what) == std::string("C"))
    return;

  switch(_dbg.source[0]){
    break; case '=':{
      // TODO ?
      // This represent the description of the source in user dependant manner?
    }

    break; case '@':
      _file_path = &_dbg.source[1];

  }
  
  _is_lua_function = true;
  _function_name = _dbg.name? _dbg.name: "";
  _line_defined = _dbg.linedefined;
  _last_line_defined = _dbg.lastlinedefined;
}
#endif


bool function_debug_info::get_function_name(I_string_store* str) const{
  str->clear();
  if(!_is_lua_function)
    return false;

  str->append(_function_name.c_str());
  return true;
}

bool function_debug_info::get_file_path(I_string_store* str) const{
  str->clear();
  if(!_is_lua_function)
    return false;

  str->append(_file_path.c_str());
  return true;
}

int function_debug_info::get_line_defined() const{
  return _line_defined;
}

int function_debug_info::get_last_line_defined() const{
  return _last_line_defined;
}


int function_debug_info::get_parameter_name_count() const{
  return _parameter_name_list.size();
}

void function_debug_info::get_parameter_name(int i, I_string_store* str) const{
  str->clear();
  if(i < 0 || i >= _parameter_name_list.size())
    return;

  str->append(_parameter_name_list[i].c_str());
}


bool function_debug_info::is_lua_function() const{
  return _is_lua_function;
}


#ifdef LUA_CODE_EXISTS

// MARK: DLL functions

DLLEXPORT I_function_debug_info* CPPLUA_CREATE_FUNCTION_DEBUG_INFO(lua_State* state, int idx){
  return __dm->new_class_dbg<function_debug_info>(DYNAMIC_MANAGEMENT_DEBUG_DATA, state, idx);
}

DLLEXPORT void CPPLUA_DELETE_FUNCTION_DEBUG_INFO(I_function_debug_info* obj){
  __dm->delete_class_dbg(obj, DYNAMIC_MANAGEMENT_DEBUG_DATA);
}

#endif