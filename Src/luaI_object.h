#ifndef I_LUA_OBJECT_HEADER
#define I_LUA_OBJECT_HEADER

#include "luaapi_compilation_context.h"


namespace lua{
  class I_vararr;

  class I_debuggable_object{
    // later implementation
  };

  // Since this object will be collected by Lua as "garbage", it is a good practice to let it "leak" in C/C++ scope
  // Also, do not create the object in stack memory as it will be put in Lua
  class I_object{
    public:
      typedef void (*object_destructor_func)(I_object* object);
      typedef void (*lua_function)(I_object* object, const I_vararr* parameters, I_vararr* results);

      virtual ~I_object(){}

      virtual object_destructor_func get_this_destructor() const = 0;

      virtual int get_function_count() const = 0;
      virtual const char* get_function_name(int idx) const = 0;
      virtual lua_function get_function(int idx) const = 0;

      virtual I_debuggable_object* as_debug_object() = 0;

      // top most table is the object representation of the lua object
      virtual void on_object_added(void* state_interface, const lua::api::compilation_context* context) = 0;
  };
}

#endif                                                              