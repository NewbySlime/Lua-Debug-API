#ifndef LUALIB_HANDLER_HEADER
#define LUALIB_HANDLER_HEADER

#include "I_debug_user.h"
#include "lua_includes.h"
#include "luaI_object.h"


namespace lua{
  class I_lib_loader{
    public:
      virtual bool load_library(const char* lib_name, I_object* lib_object) = 0;
      virtual I_object* get_library_object(const char* lib_name) const = 0;
  };

  class lib_loader: public I_lib_loader, public I_debug_user{
    private:
      I_logger* _logger;

      lua_State* _this_state;

      static int _requiref_cb(lua_State* state);

      static void _set_attached_object(lua_State* state, lib_loader* object);

    public:
      lib_loader(lua_State* state);
      ~lib_loader();

      static lib_loader* get_attached_object(lua_State* state);

      bool load_library(const char* lib_name, I_object* lib_object) override;
      I_object* get_library_object(const char* lib_name) const override;

      void set_logger(I_logger* logger) override;
  };
}

#endif