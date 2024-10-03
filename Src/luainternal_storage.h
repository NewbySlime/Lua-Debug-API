#ifndef LUA_INTERNAL_STORAGE_HEADER
#define LUA_INTERNAL_STORAGE_HEADER

#include "luaapi_core.h"


// This system is a place for storing internal data (in Lua table format). Reason for this system is not only to store internal data in Lua codespace, it is also used with lua::variable_watcher to let it ignore the internal storage to be watched by it.

namespace lua::internal{
#ifdef LUA_CODE_EXISTS
  // Only initiates the internal storage. Internal storage will not be pushed to stack. Will not replace the internal storage once it already initiated.
  void initiate_internal_storage(lua_State* state);
  // Pushes the internal storage to the stack. Will also create the internal storage if not yet initialized.
  void require_internal_storage(lua_State* state);
#endif // LUA_CODE_EXISTS
}

#endif