#ifndef LUALIBRARY_LOADER_HEADER
#define LUALIBRARY_LOADER_HEADER

#include "defines.h"
#include "I_debug_user.h"
#include "luaincludes.h"
#include "luaI_object.h"


namespace lua{
  // [Not Thread-Safe]
  class I_library_loader{
    public:
      virtual bool load_library(const char* lib_name, I_object* lib_object) = 0;
      virtual I_object* get_library_object(const char* lib_name) const = 0;
  };


#ifdef LUA_CODE_EXISTS

  class library_loader: public I_library_loader, public I_debug_user{
    private:
      I_logger* _logger;

      lua_State* _this_state;

      static int _requiref_cb(lua_State* state);

      static void _set_attached_object(lua_State* state, library_loader* object);

      static std::string _get_reference_key(void* pointer);

    public:
      library_loader(lua_State* state);
      ~library_loader();

      static library_loader* get_attached_object(lua_State* state);

      // Since the library is a part of Lua object, do not delete the object explicitly. Instead, register the deconstructor function through the object.
      bool load_library(const char* lib_name, I_object* lib_object) override;
      I_object* get_library_object(const char* lib_name) const override;

      void set_logger(I_logger* logger) override;
  };

#endif // LUA_CODE_EXISTS

}


#if (__linux)
// Just to make the compiler happy
#define __stdcall 
#endif


#define CPPLUA_CREATE_LIBRARY_LOADER cpplua_create_library_loader
#define CPPLUA_CREATE_LIBRARY_LOADER_STR MACRO_TO_STR_EXP(CPPLUA_CREATE_LIBRARY_LOADER)

#define CPPLUA_DELETE_LIBRARY_LOADER cpplua_delete_library_loader
#define CPPLUA_DELETE_LIBRARY_LOADER_STR MACRO_TO_STR_EXP(CPPLUA_DELETE_LIBRARY_LOADER)

typedef lua::I_library_loader* (__stdcall *ll_create_func)(void* istate);
typedef void (__stdcall *ll_delete_func)(lua::I_library_loader* object);

#ifdef LUA_CODE_EXISTS
DLLEXPORT lua::I_library_loader* CPPLUA_CREATE_LIBRARY_LOADER(void* istate);
DLLEXPORT void CPPLUA_DELETE_LIBRARY_LOADER(lua::I_library_loader* object);
#endif // LUA_CODE_EXISTS

#endif