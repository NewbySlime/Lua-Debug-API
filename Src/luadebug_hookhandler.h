#ifndef LUADEBUG_HOOKHANDLER_HEADER
#define LUADEBUG_HOOKHANDLER_HEADER

#include "I_debug_user.h"
#include "library_linking.h"
#include "luaincludes.h"

#include "map"


#define LUADEBUG_DEFAULTHOOKCOUNT 3


// This code will be statically bind to the compilation file
// If a code returns an interface (I_xx) create a copy with using statically linked compilation function if the code that returns comes from dynamic library

namespace lua::debug{
  class I_hook_handler: public I_debug_user{
    public:
      typedef void (*hookcb)(lua_State* state, void* cbdata);

      virtual ~I_hook_handler(){};

      virtual void set_hook(int hook_mask, hookcb cb, void* attached_obj) = 0;
      virtual void remove_hook(void* attached_obj) = 0;

      virtual void set_count(int count) = 0;
      virtual int get_count() const = 0;

      virtual const lua_Debug* get_current_debug_value() const = 0;
  };


#ifdef LUA_CODE_EXISTS

  // NOTE: lua_State shouldn't be destroyed in this class' lifetime
  class hook_handler: public I_hook_handler{
    private:
      std::map<void*, hookcb>
        _call_hook_set,
        _return_hook_set,
        _line_hook_set,
        _count_hook_set
      ;


      // NOTE: this obj will only available when _on_hook_event lifetime not ended
      // whichever case, this object shouldn't be accessed outside hook event context
      lua_Debug* _current_dbg;
      lua_State* _this_state;

      I_logger* _logger;

      int count;

      void _update_hook_config();
      void _on_hook_event(lua_State* state, lua_Debug* dbg);

      static void _on_hook_event_static(lua_State* state, lua_Debug* dbg);

    public:
      hook_handler(lua_State* state, int count = LUADEBUG_DEFAULTHOOKCOUNT);
      ~hook_handler();

      static hook_handler* get_this_attached(lua_State* state);

      void set_hook(int hook_mask, hookcb cb, void* attached_obj) override;
      void remove_hook(void* attached_obj) override;

      void set_count(int count) override;
      int get_count() const override;

      const lua_Debug* get_current_debug_value() const override;

      void set_logger(I_logger* logger) override;
  };
  
#endif // LUA_CODE_EXISTS

}

#endif