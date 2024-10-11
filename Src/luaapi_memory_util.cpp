#include "luaapi_memory_util.h"
#include "luamemory_util.h"

using namespace lua::api;
using namespace lua::memory;


#ifdef LUA_CODE_EXISTS

class _api_memory_util_definition: public lua::api::I_memory_util{
  public:
    const ::memory::I_dynamic_management* get_memory_manager() override{
      return lua::memory::get_memory_manager();
    }


    const ::memory::memory_management_config* get_memory_manager_config() override{
      return lua::memory::get_memory_manager_config();
    }

    void set_memory_manager_config(const ::memory::memory_management_config* config) override{
      lua::memory::set_memory_manager_config(config);
    }
};


static _api_memory_util_definition __api_def;

DLLEXPORT lua::api::I_memory_util* CPPLUA_GET_API_MEMORY_UTIL_DEFINITION(){
  return &__api_def;
}

#endif // LUA_CODE_EXISTS