#include "luaapi_table_util.h"

using namespace lua;
using namespace lua::api;


#ifdef LUA_CODE_EXISTS

class _api_table_util_definition: public lua::api::I_table_util{
  public:
    void iterate_table(void* istate, int stack_idx, lua::table_iter_callback callback, void* cb_data) override{lua::iterate_table((lua_State*)istate, stack_idx, callback, cb_data);}

    size_t table_len(void* istate, int stack_idx) override{return lua::table_len((lua_State*)istate, stack_idx);}

    lua::I_variant* get_table_value(void* istate, int stack_idx, const lua::I_variant* key) override{
      lua::variant* _key_var = cpplua_create_var_copy(key);
      lua::I_variant* _res_var = lua::get_table_value((lua_State*)istate, stack_idx, _key_var);

      cpplua_delete_variant(_key_var);
      return _res_var;
    }

    void set_table_value(void* istate, int stack_idx, const lua::I_variant* key, const lua::I_variant* value) override{
      lua::variant* _key_var = cpplua_create_var_copy(key);
      lua::variant* _value_var = cpplua_create_var_copy(value);

      lua::set_table_value((lua_State*)istate, stack_idx, _key_var, _value_var);

      cpplua_delete_variant(_key_var);
      cpplua_delete_variant(_value_var);
    }
};


static _api_table_util_definition __api_def;

DLLEXPORT lua::api::I_table_util* CPPLUA_GET_API_TABLE_UTIL_DEFINITION(){
  return &__api_def;
}

#endif // LUA_CODE_EXISTS