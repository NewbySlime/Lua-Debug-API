#ifndef LUALIBRARY_LOADER_HEADER
#define LUALIBRARY_LOADER_HEADER

#include "I_debug_user.h"
#include "luaincludes.h"
#include "luaI_object.h"


namespace lua{
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

    public:
      library_loader(lua_State* state);
      ~library_loader();

      static library_loader* get_attached_object(lua_State* state);

      bool load_library(const char* lib_name, I_object* lib_object) override;
      I_object* get_library_object(const char* lib_name) const override;

      void set_logger(I_logger* logger) override;
  };

#endif // LUA_CODE_EXISTS

}

#endif