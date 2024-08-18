#ifndef LUADEBUG_EXCUTIONFLOW
#define LUADEBUG_EXCUTIONFLOW

#include "I_debug_user.h"
#include "I_logger.h"
#include "lua_includes.h"
#include "luadebug_hookhandler.h"

#include "condition_variable"
#include "mutex"
#include "stack"
#include "thread"


namespace lua::debug{
  // NOTE: This will block the running thread for lua runtime. Preferably to use multi threading for lua and main processing.
  //  lua_State shouldn't be destroyed in this class' lifetime
  class execution_flow: public I_debug_user{
    public:
      enum step_type{
        st_none,
        st_per_line,
        st_per_counts,
        st_in,
        st_out,
        st_over
      };

    private:
      struct _function_data{
        std::string name;
        bool is_tailcall;
      };

      hook_handler* _hook_handler;
      lua_State* _this_state;

      I_logger* _logger;


      bool _do_block = false;

      std::mutex _execution_mutex;
      std::condition_variable _execution_block;
      bool _currently_executing = true;

      step_type _stepping_type = st_none;
      unsigned int _step_layer_check;

      int _current_line;
      std::stack<_function_data*> _function_layer;

      void _block_execution();

      void _hookcb();

      static void _set_bind_obj(execution_flow* obj, lua_State* state);

      static void _hookcb_static(lua_State* state, void* cb_data);

    public:
      execution_flow(lua_State* state);
      ~execution_flow();

      static execution_flow* get_attached_obj(lua_State* state);

      void set_step_count(int count);
      int get_step_count() const;

      unsigned int get_function_layer() const;
      std::string get_function_name() const;

      void block_execution();
      void resume_execution();
      void step_execution(step_type st);

      bool currently_pausing();

      const lua_Debug* get_debug_data() const;

      void set_logger(I_logger* logger) override;
  };
}

#endif