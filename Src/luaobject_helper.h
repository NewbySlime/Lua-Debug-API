#ifndef LUAOBJECT_HELPER_HEADER
#define LUAOBJECT_HELPER_HEADER

#include "lua_includes.h"
#include "lua_variant.h"


// When function to be called wants to throw a Lua error, a user function should add error_var as the first result value.
// If function does not want to throw, but wanted to return an error value, the function should return it after the first result value (can be padded with nil_var).

namespace lua{
  namespace object{
    struct fdata{
      public:
        const char* name;
        lua::I_object::lua_function func;

        fdata(const char* fname, lua::I_object::lua_function func){
          name = fname;
          this->func = func;
        }
    };

    class function_store: virtual public I_object{
      private:
        const fdata* _function_list = NULL;

        int _func_count;


      protected:
        object_destructor_func _deinit_func;

        // NOTE: for array termination, it uses fdata object that has NULL values
        void set_function_data(const fdata* list);

      public:
        function_store(object_destructor_func destructor);

        void set_destructor(object_destructor_func destructor);

        object_destructor_func get_this_destructor() const override;

        int get_function_count() const override;
        const char* get_function_name(int idx) const override;
        lua_function get_function(int idx) const override;
        
    };


    I_object* get_obj_from_table(lua_State* state, int table_idx);
    void push_object_to_table(lua_State* state, I_object* object, int table_idx);
  }
};

#endif