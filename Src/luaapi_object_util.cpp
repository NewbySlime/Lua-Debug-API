#include "luaapi_object_util.h"
#include "luaobject_util.h"

using namespace lua;
using namespace lua::api;
using namespace lua::object;


#ifdef LUA_CODE_EXISTS

class _api_object_util_definition: public I_object_util{
  public:
    I_object* get_object_from_table(void* istate, int table_idx) override{return lua::object::get_object_from_table((lua_State*)istate, table_idx);}
    void push_object_to_table(void* istate, I_object* object, int table_idx) override{return lua::object::push_object_to_table((lua_State*)istate, object, table_idx);}
};


static _api_object_util_definition __api_def;

DLLEXPORT I_object_util* CPPLUA_GET_API_OBJECT_UTIL_DEFINITION(){
  return &__api_def;
}

#endif // LUA_CODE_EXISTS