#ifndef LUADEBUG_EXCUTIONFLOW
#define LUADEBUG_EXCUTIONFLOW

#include "I_debug_user.h"
#include "lua_includes.h"
#include "luadebug_hookhandler.h"

#include "mutex"
#include "thread"


namespace lua::debug{
  class execution_flow: public I_debug_user{
    public:
      enum step_type{
        st_per_line,
        st_per_counts
      };

    private:
      hook_handler* _hook_handler;
      lua_State* _this_state;

      I_logger* _logger;


      std::thread::id _execution_id;
      std::mutex _execution_block_line_m;
      std::mutex _execution_block_count_m;

      int _current_count;


      static void _set_bind_obj(execution_flow* obj, lua_State* state);

      static void _hookcb(lua_State* state, void* cb_data);

    public:
      execution_flow(lua_State* state);
      ~execution_flow();

      static execution_flow* get_attached_obj(lua_State* state);

      void set_step_count(int count);
      int get_step_count();

      void block_execution();
      void resume_execution();
      void step_execution(step_type st);

      void set_logger(I_logger* logger) override;
  };
}

#endif