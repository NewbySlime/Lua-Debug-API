#ifndef LUAAPI_CORE_HEADER
#define LUAAPI_CORE_HEADER

#include "luaapi_compilation_context.h"


namespace lua::api{
  struct core{
    void* istate = NULL;
    const compilation_context* context = NULL;
  };
}

#endif