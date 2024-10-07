#include "luaapi_runtime.h"

using namespace lua;
using namespace lua::api;
using namespace lua::debug;
using namespace lua::global;


#ifdef LUA_CODE_EXISTS

class _api_runtime_function: public I_runtime{
  public:
    lua::global::I_print_override* create_print_override(void* istate) override{
      return cpplua_create_global_print_override(istate);
    }

    void delete_print_override(lua::global::I_print_override* object) override{
      cpplua_delete_global_print_override(object);
    }

    lua::I_library_loader* create_library_loader(void* istate) override{
      return cpplua_create_library_loader(istate);
    }

    void delete_library_loader(lua::I_library_loader* object) override{
      cpplua_delete_library_loader(object);
    }

    lua::I_runtime_handler* create_runtime_handler(const char* lua_path, bool immediate_run, bool load_library) override{
      return cpplua_create_runtime_handler(lua_path, immediate_run, load_library);
    }

    void delete_runtime_handler(lua::I_runtime_handler* object) override{
      return cpplua_delete_runtime_handler(object);
    }
};


static _api_runtime_function __api_def;

DLLEXPORT lua::api::I_runtime* CPPLUA_GET_API_RUNTIME_DEFINITION(){
  return &__api_def;
}

#endif // LUA_CODE_EXISTS