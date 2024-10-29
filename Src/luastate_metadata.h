#ifndef LUASTATE_METADATA_HEADER
#define LUASTATE_METADATA_HEADER

#ifdef LUA_CODE_EXISTS

#include "luaI_object.h"
#include "luaincludes.h"
#include "luaobject_util.h"


namespace lua::state{
  // All object stored will be automatically deleted by the function stored in I_object. Different key that has same I_object is allowed as it will not be deleted more than once.
  // WARN: Do not set an object that are already pushed to Lua code space as this code will not use Lua functions to store and assuming the stored objects' lifetime are only handled in the metadata object.
  // [Not Thread-Safe, but synchronization are provided in the object]
  class I_metadata{
    public:
      virtual ~I_metadata(){}

      virtual lua::I_object* get_object(const char* key) const = 0;
      virtual void set_object(const char* key, lua::I_object* obj) = 0;
      virtual void remove_object(const char* key) = 0;
      virtual bool has_object(const char* key) const = 0;

      virtual void lock_metadata() = 0;
      virtual void unlock_metadata() = 0;
  };


  // Metadata will be stored in extra space. The object will not be stored in global or internal storage.
  I_metadata* initiate_metadata(lua_State* state);
  void deinitiate_metadata(lua_State* state);
  I_metadata* get_metadata(lua_State* state);
}


#endif // LUA_CODE_EXISTS

#endif