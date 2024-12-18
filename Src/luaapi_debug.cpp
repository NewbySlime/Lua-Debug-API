#include "luaapi_debug.h"
#include "luamemory_util.h"

using namespace lua::api;
using namespace lua::debug;
using namespace lua::memory;
using namespace ::memory;


#ifdef LUA_CODE_EXISTS

static const I_dynamic_management* __dm = get_memory_manager(); 


class _api_debug_function: public I_debug{
  public:
    lua_Hook gethook(void* istate) override{return lua_gethook((lua_State*)istate);}
    int gethookcount(void* istate) override{return lua_gethookmask((lua_State*)istate);}
    int gethookmask(void* istate) override{return lua_gethookmask((lua_State*)istate);}
    int getinfo(void* istate, const char* what, void* dbg_obj) override{return lua_getinfo((lua_State*)istate, what, (lua_Debug*)dbg_obj);}
    const char* getlocal(void* istate, const void* dbg_obj, int n) override{return lua_getlocal((lua_State*)istate, (const lua_Debug*)dbg_obj, n);}
    int getstack(void* istate, int level, void* dbg_obj) override{return lua_getstack((lua_State*)istate, level, (lua_Debug*)dbg_obj);}
    const char* getupvalue(void* istate, int fidx, int n) override{return lua_getupvalue((lua_State*)istate, fidx, n);}
    void sethook(void* istate, lua_Hook f, int mask, int count) override{lua_sethook((lua_State*)istate, f, mask, count);}
    const char* setlocal(void* istate, const void* dbg_obj, int n) override{return lua_setlocal((lua_State*)istate, (const lua_Debug*)dbg_obj, n);}
    const char* setupvalue(void* istate, int fidx, int n) override{return lua_setupvalue((lua_State*)istate, fidx, n);}
    void* upvalueid(void* istate, int fidx, int n) override{return lua_upvalueid((lua_State*)istate, fidx, n);}
    void upvaluejoin(void* istate, int fidx1, int n1, int fidx2, int n2) override{lua_upvaluejoin((lua_State*)istate, fidx1, n1, fidx2, n2);}

    void* create_lua_debug_obj() override{
      return __dm->malloc(sizeof(lua_Debug), DYNAMIC_MANAGEMENT_DEBUG_DATA);
    }

    void delete_lua_debug_obj(void* dbg_obj) override{
      __dm->free(dbg_obj, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    }

    I_variable_watcher* create_variable_watcher(void* istate) override{
      return cpplua_create_variable_watcher(istate);
    }

    void delete_variable_watcher(I_variable_watcher* object) override{
      cpplua_delete_variable_watcher(object);
    }

    I_file_info* create_file_info(const char* file_path) override{
      return cpplua_create_file_info(file_path);
    }

    void delete_file_info(I_file_info* obj) override{
      cpplua_delete_file_info(obj);
    }
};


static _api_debug_function __api_def;

DLLEXPORT lua::api::I_debug* CPPLUA_GET_API_DEBUG_DEFINITION(){
  return &__api_def;
}

#endif // LUA_CODE_EXISTS