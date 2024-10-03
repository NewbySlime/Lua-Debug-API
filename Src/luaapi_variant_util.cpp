#include "luaapi_variant_util.h"
#include "luavariant_util.h"

using namespace lua;
using namespace lua::api;

#ifdef LUA_CODE_EXISTS

class _api_variant_util_definition: public lua::api::I_variant_util{
  public:
    I_variant* to_variant(void* istate, int stack_idx) override{return lua::to_variant((lua_State*)istate, stack_idx);}
    I_variant* to_variant_fglobal(void* istate, const char* name) override{return lua::to_variant_fglobal((lua_State*)istate, name);}
    I_variant* to_variant_ref(void* istate, int stack_idx) override{return lua::to_variant_ref((lua_State*)istate, stack_idx);}

    void set_special_type(void* istate, int stack_idx, int new_type) override{lua::set_special_type((lua_State*)istate, stack_idx, new_type);}
    int get_special_type(void* istate, int stack_idx) override{return lua::get_special_type((lua_State*)istate, stack_idx);}

    void set_global(void* istate, const char* global_name, I_variant* var){lua::set_global((lua_State*)istate, global_name, var);}

    I_variant* create_variant_copy(const I_variant* var) override{return cpplua_create_var_copy(var);}
    void delete_variant(const I_variant* var) override{cpplua_delete_variant(var);}

    void set_default_logger(I_logger* logger) override{return cpplua_variant_set_default_logger(logger);}
    

    void to_string(void* istate, int stack_idx, I_string_store* result) override{
      std::string _result = lua::to_string((lua_State*)istate, stack_idx);
      result->append(_result.c_str());
    }
};


static _api_variant_util_definition __api_def;

DLLEXPORT I_variant_util* CPPLUA_GET_API_VARIANT_UTIL_DEFINITION(){
  return &__api_def;
}

#endif // LUA_CODE_EXISTS