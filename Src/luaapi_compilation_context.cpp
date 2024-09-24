#include "luaapi_compilation_context.h"


using namespace lua::api;


compilation_context::compilation_context(I_stack* stack, I_value* value, I_table_util* tableutil, I_variant_util* varutil){
  api_stack = stack;
  api_value = value;
  api_tableutil = tableutil;
  api_varutil = varutil;
}


static const lua::api::compilation_context __compilation_context(
  cpplua_get_api_stack_definition(),
  cpplua_get_api_value_definition(),
  cpplua_get_api_table_util_definition(),
  cpplua_get_api_variant_util_definition()
);

DLLEXPORT const lua::api::compilation_context* CPPLUA_GET_API_COMPILATION_CONTEXT(){
  return &__compilation_context;
}