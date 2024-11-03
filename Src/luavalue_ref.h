#ifndef LUA_VALUE_REF_HEADER
#define LUA_VALUE_REF_HEADER

#include "luaapi_core.h"
#include "string"

namespace lua{
  // NOTE: "reference initiated" means that the reference (metadata) already created or reference has been incremented from the current reference object at the time of the reference object contruction.
  //  This is different from "reference created", which means the metadata for storing the referencing data hasn't been created.
  //  If the reference never updated/initiated, value_ref cannot hold the reference. Which any value_ref can delete the reference metadata even when a value_ref explicitly uses those reference keys.
  // [Thread-Safe, only uses Lua's API]
  class value_ref{
    private:
      std::string _ref_key;
      std::string _value_key;

      lua::api::core _lc;

      bool _reference_initiated = false;

      void _require_internal_data();

      // LUA_PARAM 1: Value to be set as the reference. (Not popped)
      // This will not push the metadata object.
      // WARN: Make sure that the metadata has not yet created, as this function could replace the current metadata.
      void _create_metadata();
      // Will not trigger Lua error even if the metadata hasn't been created.
      void _delete_metadata();
      // Pushed value can be nil if metadata has not yet been created.
      void _push_metadata();

      // WARN: Make sure the metadata has been created, as this function could trigger a Lua error.
      void _increment_reference();
      // WARN: Make sure the metadata has been created, as this function could trigger a Lua error.
      void _decrement_reference();

      // WARN: Make sure the metadata has been created, as this function could trigger a Lua error.
      int _get_reference_count();

      void _initiate_class();

    public:
      // This will not immediately create the reference, but it will create one when the value is set. Unless the reference are already created.
      value_ref(const lua::api::core* lua_core, const char* reference_key, const char* value_key);
      value_ref(const value_ref& ref);
      ~value_ref();

      // Checks if the reference has been created in Lua.
      bool has_reference();

      // Initiate the reference if not yet. Only works when has_reference() is true but reference_initiated() is false.
      void update_reference();
      // Checks if this object has initiate the reference.
      bool reference_initiated();

      // If reference has not been created, the pushed value will be nil.
      // The function will update the reference if necessary.
      void push_value_to_stack();
      // The function will initiate/update the reference if necessary. Arguments will not be popped from the stack.
      // WARN: this will replace the value across references.
      void set_value(int value_sidx);

      const lua::api::core* get_lua_core();
  };
}

#endif