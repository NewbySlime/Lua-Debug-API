#ifndef LUA_VARIANT_HEADER
#define LUA_VARIANT_HEADER

#include "I_logger.h"
#include "library_linking.h"
#include "lua_includes.h"
#include "luaapi_stack.h"
#include "luaapi_value.h"
#include "luaI_object.h"
#include "macro_helper.h"
#include "string_store.h"

#include "map"
#include "string"


#define LUA_TERROR 0x101
#define LUA_TCPPOBJECT 0x102


// This code will be statically bind to the compilation file
// If a code returns an interface (I_xx) create a copy with using statically linked compilation function if the code that returns comes from dynamic library

namespace lua{
  void set_default_logger(I_logger* logger);

  class comparison_variant;
  class I_vararr;
  class vararr;

  namespace api{
    class I_table_util;
    class I_variant_util;
  }



  // MARK: variant

  class I_variant{
    public:
      I_variant(){};
      virtual ~I_variant(){};

      virtual int get_type() const = 0;
      constexpr static int get_static_lua_type(){return LUA_TNIL;}

      virtual void to_string(I_string_store* pstring) const = 0;
  };

  class variant: virtual public I_variant{
    protected:
      // passed value will always be the same with the inherited type
      virtual int _compare_with(const variant* rhs) const{return 0;}

    public:
      variant();

      int get_type() const override;

      virtual bool from_state(lua_State* state, int stack_index){return false;}
      virtual void push_to_stack(lua_State* state) const {lua_pushnil(state);}
      virtual variant* create_copy() const {return new variant();}

      void to_string(I_string_store* pstring) const override;
      virtual std::string to_string() const;

      // the lhs is itself
      int compare_with(const variant* rhs) const;

      void setglobal(lua_State* state, std::string var_name);
      void setglobal(lua_State* state, const char* var_name);
  };




  // MARK: nil_var

  class I_nil_var: virtual public I_variant{
    public:
      constexpr static int get_static_lua_type(){return LUA_TNIL;}
  };

  class nil_var: public variant, public I_nil_var{};



  
  // MARK: string_var

  class I_string_var: virtual public I_variant{
    public:
      constexpr static int get_static_lua_type(){return LUA_TSTRING;}

      virtual void append(const char* cstr) = 0;
      virtual void append(const char* cstr, std::size_t len) = 0;

      virtual const char* get_string() const = 0;
      virtual std::size_t get_length() const = 0;
  };

  class string_var: public variant, public I_string_var{
    private:
      char* _str_mem = NULL;
      std::size_t _mem_size = 0;

      void _init_class();

      void _put_delimiter_at_end();

      void _resize_mem(std::size_t newlen);

      void _set_to_cstr(const char* cstr, std::size_t strlen);
      void _append_cstr(const char* cstr, std::size_t strlen);

      static int _strcmp(const string_var& var1, const string_var& var2);

    protected:
      int _compare_with(const variant* rhs) const override;

    public:
      string_var();
      string_var(const string_var& var1);
      string_var(const char* cstr);
      string_var(const char* cstr, std::size_t len);
      string_var(const std::string& str);
      string_var(lua_State* state, int stack_idx);
      ~string_var();

      int get_type() const override;

      static string_var* create_copy_static(const I_string_var* data);

      bool from_state(lua_State* state, int stack_idx) override;
      void push_to_stack(lua_State* state) const override;
      variant* create_copy() const override;

      void to_string(I_string_store* pstring) const override;
      std::string to_string() const override;

      string_var& operator+=(const string_var& var1);
      string_var& operator+=(const std::string& var1);
      string_var& operator+=(const char* var1);

      string_var operator+(const string_var& var1) const;
      string_var operator+(const std::string& var1) const;
      string_var operator+(const char* var1) const;

      const char& operator[](std::size_t i) const;

      bool operator<(const string_var& var1) const;
      bool operator>(const string_var& var1) const;
      bool operator<=(const string_var& var1) const;
      bool operator>=(const string_var& var1) const;
      bool operator==(const string_var& var1) const;
      
      void append(const string_var& str);
      void append(const std::string& str);
      void append(const char* cstr) override;
      void append(const char* cstr, std::size_t len) override;

      const char* get_string() const override;
      std::size_t get_length() const override;
  };


  
  // MARK: number_var

  class I_number_var: virtual public I_variant{
    public:
      constexpr static int get_static_lua_type(){return LUA_TNUMBER;}

      virtual double get_number() const = 0;
      virtual void set_number(double num) = 0;
  };

  class number_var: public variant, public I_number_var{
    private:
      lua_Number _this_data;

      static int _comp_num(const number_var& var1, const number_var& var2);

    protected:
      int _compare_with(const variant* rhs) const override;

    public:
      number_var();
      number_var(const double& from);
      number_var(lua_State* state, int stack_idx);

      int get_type() const override;

      static number_var* create_copy_static(const I_number_var* data);
      
      bool from_state(lua_State* state, int stack_idx) override;
      void push_to_stack(lua_State* state) const override;
      variant* create_copy() const override;

      void to_string(I_string_store* data) const override;
      std::string to_string() const override;

      double get_number() const override;
      void set_number(double num) override;

      number_var& operator=(const number_var& val);
      number_var& operator=(const double& val);

      number_var& operator+=(const number_var& var1);
      number_var& operator-=(const number_var& var1);
      number_var& operator*=(const number_var& var1);
      number_var& operator/=(const number_var& var1);

      number_var operator+(const number_var& var1) const;
      number_var operator-(const number_var& var1) const;
      number_var operator*(const number_var& var1) const;
      number_var operator/(const number_var& var1) const;

      bool operator==(const number_var& var1) const;
      bool operator!=(const number_var& var1) const;
      bool operator<(const number_var& var1) const;
      bool operator>(const number_var& var1) const;
      bool operator<=(const number_var& var1) const;
      bool operator>=(const number_var& var1) const;
  };



  // MARK: bool_var

  class I_bool_var: virtual public I_variant{
    public:
      constexpr static int get_static_lua_type(){return LUA_TBOOLEAN;}

      virtual bool get_boolean() const = 0;
      virtual void set_boolean(bool b) = 0;
  };

  class bool_var: public variant, public I_bool_var{
    private:
      bool _this_data;

      static int _comp_bool(const bool_var& var1, const bool_var& var2);

    protected:
      int _compare_with(const variant* rhs) const override;

    public:
      bool_var();
      bool_var(const bool& from);
      bool_var(lua_State* state, int stack_idx);

      int get_type() const override;

      static bool_var* create_copy_static(const I_bool_var* data);
      
      bool from_state(lua_State* state, int stack_idx) override;
      void push_to_stack(lua_State* state) const override;
      variant* create_copy() const override;

      void to_string(I_string_store* data) const override;
      std::string to_string() const override;

      bool get_boolean() const override;
      void set_boolean(bool b) override;

      bool_var& operator=(const bool_var& val);
      bool_var& operator=(const bool& val);

      bool operator==(const bool_var& var1) const;
      bool operator!=(const bool_var& var1) const;
      bool operator<(const bool_var& var1) const;
      bool operator>(const bool_var& var1) const;
      bool operator<=(const bool_var& var1) const;
      bool operator>=(const bool_var& var1) const;

      bool operator!() const;
  };


  
  // MARK: table_var
  // NOTE: Every lua::variant copy it returns, make sure to delete using the same compilation code the same as table_var created with.

  class I_table_var: virtual public I_variant{
    public:
      constexpr static int get_static_lua_type(){return LUA_TTABLE;}

      virtual I_table_var* create_table_copy() const = 0;

      // returns NULL-terminated array
      virtual const I_variant** get_keys() const = 0;
      virtual void update_keys() = 0;

      // This will create a copy
      virtual I_variant* get_value(const I_variant* key) = 0;
      // This will create a copy
      virtual const I_variant* get_value(const I_variant* key) const = 0;

      virtual void set_value(const I_variant* key, const I_variant* data) = 0;

      virtual bool remove_value(const I_variant* key) = 0;

      virtual bool is_reference() const = 0;
      virtual const void* get_table_pointer() const = 0;
      virtual void* get_lua_interface() const = 0;
      virtual lua::api::I_value* get_lua_value_api_interface() const = 0;
      virtual lua::api::I_stack* get_lua_stack_api_interface() const = 0;
      virtual lua::api::I_table_util* get_lua_table_api_interface() const = 0;
      virtual lua::api::I_variant_util* get_lua_variant_api_interface() const = 0;
  };

  // Can parse Global table, but it will skip "_G" value
  class table_var: public variant, public I_table_var{
    private:
      const I_variant** _keys_buffer;

      bool _is_ref = true;

      std::map<comparison_variant, variant*>* _table_data = NULL;

      void* _lua_state;
      const void* _table_pointer = NULL;

      lua::api::I_stack* _api_stack = NULL;
      lua::api::I_value* _api_value = NULL;

      lua::api::I_variant_util* _api_var = NULL;
      lua::api::I_table_util* _api_table = NULL;

      void _init_class();

      void _copy_from_var(const table_var& var);
      void _clear_table_data();

      // LUA PARAM: the table value should be pushed (as the top most) beforehand
      //  it will not pop the table value
      // NOTE: this will replace the metadata with the new one, to add a reference, use _increment_reference()
      void _create_reference();
      // NOTE: this will remove the metadata value. Use _reference_count() and _decrement_reference() for remove a reference but don't remove the metadata.
      void _delete_reference();
      // if no ref table in the global field, it will create a new one
      // pushes the global metadata list as the top stack [+0,+1,-]
      void _require_global_table() const;

      // pushes metadata for this table reference at the top [+0,+1,-]
      // WARN: Usage of this function is prohibited when the metadata is not prepared. For creating metadata reference, see _create_reference().
      void _push_metadata_table() const;
      // pushes the actual table data at the top [+0,+1,-]
      // WARN: Usage of this function is prohibited when the metadata is not prepared. For creating metadata reference, see _create_reference().
      void _push_actual_table() const;

      void _increment_reference();
      void _decrement_reference();
      int _get_reference_count();

      bool _has_reference_metadata();

      static std::string _get_reference_key(const void* pointer);

      // used for creating copy from lua_State
      static void _fs_iter_cb(lua_State* state, int key_stack, int val_stack, int iter_idx, void* cb_data);
      // needed api interface for use:
      //  lua::api::I_table_util
      //  lua::api::I_value
      //  lua::api::I_variant_util
      bool _from_state_copy_by_interface(void* istate, int stack_idx);

      variant* _get_value_by_interface(const I_variant* key) const;
      void _set_value_by_interface(const I_variant* key, const I_variant* value);
      bool _remove_value_by_interface(const I_variant* key);

      variant* _get_value(const comparison_variant& comp_var) const;
      variant* _get_value(const I_variant* key) const;

      void _set_value(const comparison_variant& comp_key, const variant* value);
      void _set_value(const I_variant* key, const I_variant* value);

      bool _remove_value(const comparison_variant& comp_key);
      bool _remove_value(const I_variant* key);

      void _init_keys_reg();
      void _update_keys_reg();
      void _clear_keys_reg();

    public:
      table_var();
      table_var(const table_var& var);
      table_var(lua_State* state, int stack_idx);
      ~table_var();

      int get_type() const override;

      static table_var* create_copy_static(const I_table_var* data);

      bool from_state(lua_State* state, int stack_idx) override;
      bool from_state_copy(lua_State* state, int stack_idx);

      void push_to_stack(lua_State* state) const override;
      variant* create_copy() const override;

      void to_string(I_string_store* pstring) const override;
      std::string to_string() const override;

      I_table_var* create_table_copy() const override;

      const I_variant** get_keys() const override;
      void update_keys() override;

      variant* get_value(const comparison_variant& comp_var);
      I_variant* get_value(const I_variant* key) override;

      const variant* get_value(const comparison_variant& comp_var) const;
      const I_variant* get_value(const I_variant* key) const override;

      void set_value(const comparison_variant& comp_var, variant* value);
      void set_value(const I_variant* key, const I_variant* data) override;

      bool remove_value(const comparison_variant& comp_var);
      bool remove_value(const I_variant* key) override;

      bool is_reference() const;
      const void* get_table_pointer() const;
      void* get_lua_interface() const;
      lua::api::I_value* get_lua_value_api_interface() const;
      lua::api::I_stack* get_lua_stack_api_interface() const;
      lua::api::I_table_util* get_lua_table_api_interface() const;
      lua::api::I_variant_util* get_lua_variant_api_interface() const;
  };



  // MARK: lightuser_var

  class I_lightuser_var: virtual public I_variant{
    public:
      constexpr static int get_static_lua_type(){return LUA_TLIGHTUSERDATA;}
      
      virtual void* get_data() const = 0;
      virtual void set_data(void* data) = 0;
  };

  class lightuser_var: public variant, public I_lightuser_var{
    private:
      void* _pointer_data;

      static int _comp_lud(const lightuser_var& var1, const lightuser_var& var2);

    protected:
      int _compare_with(const variant* rhs) const override;

    public:
      lightuser_var();
      lightuser_var(void* pointer);
      lightuser_var(lua_State* state, int stack_idx);

      int get_type() const override;

      static lightuser_var* create_copy_static(const I_lightuser_var* data);

      bool from_state(lua_State* state, int stack_idx) override;
      void push_to_stack(lua_State* state) const override;
      variant* create_copy() const override;

      void to_string(I_string_store* pstring) const override;
      std::string to_string() const override;

      void* get_data() const override;
      void set_data(void* data) override;

      bool operator==(const lightuser_var& var1) const;
      bool operator!=(const lightuser_var& var1) const;
      bool operator<(const lightuser_var& var1) const;
      bool operator>(const lightuser_var& var1) const;
      bool operator<=(const lightuser_var& var1) const;
      bool operator>=(const lightuser_var& var1) const; 
  };




  // MARK: function_var
  
  class I_function_var: virtual public I_variant{
    public:
      constexpr static int get_static_lua_type(){return LUA_TFUNCTION;}

      virtual I_vararr* get_arg_closure() = 0;
      virtual const I_vararr* get_arg_closure() const = 0;

      virtual lua_CFunction get_function() const = 0;
      virtual void set_function(lua_CFunction fn) = 0;
  };

  // NOTE: for now, only cfunction that can be stored
  class function_var: public I_function_var, public variant{
    private:
      lua_CFunction _this_fn = NULL;
      vararr* _fn_args = NULL;

      void _init_class();

    protected:
      // compared results are based on the function pointer
      int _compare_with(const variant* rhs) const override;

    public:
      function_var(lua_CFunction fn, const I_vararr* args = NULL);
      function_var(lua_State* state, int stack_idx);
      ~function_var();

      int get_type() const override;

      static function_var* create_copy_static(const I_function_var* data);
      
      bool from_state(lua_State* state, int stack_idx) override;
      // NOTE: pushing to stack with bound CFunction as NULL, it will push a nil value.
      void push_to_stack(lua_State* state) const override;
      variant* create_copy() const override;

      std::string to_string() const;
      void to_string(I_string_store* pstring) const override;

      I_vararr* get_arg_closure() override;
      const I_vararr* get_arg_closure() const override;

      lua_CFunction get_function() const override;
      void set_function(lua_CFunction fn) override;
  };




  // MARK: error_var

  class I_error_var: virtual public I_variant{
    public:
      constexpr static int get_static_lua_type(){return LUA_TERROR;}

      virtual const I_variant* get_error_data_interface() const = 0;
      virtual long long get_error_code() const = 0;
  };

  class error_var: public variant, public I_error_var{
    private:
      variant* _err_data = NULL;
      long long _error_code = -1;

    public:
      error_var();
      error_var(const error_var& var);
      error_var(const variant* data, long long code);
      error_var(lua_State* state, int stack_idx);
      ~error_var();

      int get_type() const override;

      static error_var* create_copy_static(const I_error_var* data);

      bool from_state(lua_State* state, int stack_idx) override;
      void push_to_stack(lua_State* state) const override;
      variant* create_copy() const override;

      void to_string(I_string_store* pstring) const override;
      std::string to_string() const override;

      const variant* get_error_data() const;
      const I_variant* get_error_data_interface() const override;

      long long get_error_code() const override;

      void set_error_code(int error_code);
  };




  // MARK: object_var
  // See luaobject_helper.h for more information
  // NOTE: object_var does not store the actual type of the stored object

  // NOTE: Actual representation in Lua is a table with metamethods
  class I_object_var: virtual public I_variant{
    public:
      constexpr static int get_static_lua_type(){return LUA_TCPPOBJECT;}

      // Sets the new object reference
      // Returns the previous object reference
      // As long as it becomes a Lua object, it is okay to not free/delete the memory
      virtual I_object* set_object_reference(I_object* object) = 0;
      virtual I_object* get_object_reference() const = 0;

  };

  class object_var: public I_object_var, public variant{
    private:
      I_object* _obj = NULL;

    public:
      object_var();
      object_var(I_object* obj_ref);
      
      int get_type() const override;

      static object_var* create_copy_static(const I_object_var* data);

      bool from_state(lua_State* state, int stack_idx) override;
      void push_to_stack(lua_State* state) const override;
      variant* create_copy() const override;

      void to_string(I_string_store* pstring) const override;
      std::string to_string() const override;

      I_object* set_object_reference(I_object* object) override;
      I_object* get_object_reference() const override;
      
  };




  // MARK: comparison_variant

  class comparison_variant{
    private:
      variant* _this_var;

      void _init_class(const variant* from);

    public:
      comparison_variant(const variant* from);
      comparison_variant(const comparison_variant& from);
      comparison_variant(const string_var& from);
      comparison_variant(const number_var& from);
      comparison_variant(const bool_var& from);
      comparison_variant(const lightuser_var& from);
      ~comparison_variant();

      bool operator<(const comparison_variant& rhs) const;
      bool operator>(const comparison_variant& rhs) const;
      bool operator<=(const comparison_variant& rhs) const;
      bool operator>=(const comparison_variant& rhs) const;
      bool operator==(const comparison_variant& rhs) const;
      bool operator!=(const comparison_variant& rhs) const;

      const variant* get_comparison_data() const;
  };


  // This will not pop the value.
  variant* to_variant(lua_State* state, int stack_idx);

  // Get variant from determined global value. This will also pops the fetched value.
  variant* to_variant_fglobal(lua_State* state, const char* global_name);

  // Pointer value (in lua) parsed to string variant.
  variant* to_variant_ref(lua_State* state, int stack_idx);

  void push_variant(lua_State* state, const variant* var);

  string_var from_pointer_to_str(void* pointer);
  void* from_str_to_pointer(const string_var& str);


  namespace api{
    class I_variant_util{
      public:
        virtual I_variant* to_variant(void* istate, int stack_idx) = 0;
        virtual I_variant* to_variant_fglobal(void* istate, const char* global_name) = 0;
        virtual I_variant* to_variant_ref(void* istate, int stack_idx) = 0;

        // NOTE: var code functions should be the same as this interface compilation
        virtual void push_variant(void* istate, const I_variant* var) = 0;

        virtual I_variant* create_variant_copy(const I_variant* var) = 0;
        virtual void delete_variant(const I_variant* var) = 0;
    };
  }
}



// MARK: Static functions

lua::variant* cpplua_create_var_copy(const lua::I_variant* data);



// MARK: DLL functions

#define CPPLUA_VARIANT_SET_DEFAULT_LOGGER cpplua_variant_set_default_logger
#define CPPLUA_VARIANT_SET_DEFAULT_LOGGER_STR MACRO_TO_STR_EXP(CPPLUA_VARIANT_SET_DEFAULT_LOGGER)

#define CPPLUA_DELETE_VARIANT cpplua_delete_variant
#define CPPLUA_DELETE_VARIANT_STR MACRO_TO_STR_EXP(CPPLUA_DELETE_VARIANT)

#define CPPLUA_GET_TYPE_NAME cpplua_get_type_name
#define CPPLUA_GET_TYPE_NAME_STR MACRO_TO_STR_EXP(CPPLUA_GET_TYPE_NAME)

#define CPPLUA_GET_API_VARIANT_UTIL_DEFINITION cpplua_get_api_variant_util_definition
#define CPPLUA_GET_API_VARIANT_UTIL_DEFINITION_STR MACRO_TO_STR_EXP(CPPLUA_GET_API_VARIANT_UTIL_DEFINITION)

typedef void (__stdcall *var_set_def_logger_func)(I_logger* logger);
typedef void (__stdcall *del_var_func)(const lua::I_variant* data);
typedef const char* (__stdcall *get_type_name_func)(int type_name);
typedef lua::api::I_variant_util* (__stdcall *get_api_variant_util_func)();

DLLEXPORT void CPPLUA_VARIANT_SET_DEFAULT_LOGGER(I_logger* logger);
DLLEXPORT void CPPLUA_DELETE_VARIANT(const lua::I_variant* data);

DLLEXPORT const char* CPPLUA_GET_TYPE_NAME(int type_name);

DLLEXPORT lua::api::I_variant_util* CPPLUA_GET_API_VARIANT_UTIL_DEFINITION();

#endif