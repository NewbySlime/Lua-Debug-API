#include "luaapi_internal.h"
#include "luainternal_storage.h"


using namespace lua;
using namespace lua::api;
using namespace lua::internal;


#ifdef LUA_CODE_EXISTS

class _api_internal_function: public I_internal{
  public:
    void initiate_internal_storage(void* istate) override{initiate_internal_storage((lua_State*)istate);}
    void require_internal_storage(void* istate) override{require_internal_storage((lua_State*)istate);}
};


static _api_internal_function __api_def;

DLLEXPORT lua::api::I_internal* CPPLUA_GET_API_INTERNAL_DEFINITION(){
  return &__api_def;
}

#endif // LUA_CODE_EXISTS