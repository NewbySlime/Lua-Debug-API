#ifndef LUA_VARIANT_HEADER
#define LUA_VARIANT_HEADER

#include "I_logger.h"
#include "library_linking.h"
#include "luaincludes.h"
#include "luaapi_compilation_context.h"
#include "luaI_object.h"
#include "luavalue_ref.h"
#include "macro_helper.h"
#include "string_store.h"

#include "map"
#include "string"


#define LUA_TERROR 0x101
#define LUA_TCPPOBJECT 0x102

#define LUA_FUNCVAR_FUNCTION_UPVALUE 1

#define LUA_FUNCVAR_START_UPVALUE_IDX 1
#define LUA_FUNCVAR_START_UPVALUE (LUA_FUNCVAR_START_UPVALUE_IDX+1)


// This code will be statically bind to the compilation file
// If a code returns an interface (I_xx) create a copy with using statically linked compilation function if the code that returns comes from dynamic library

namespace lua{
  void set_default_logger(I_logger* logger);

  class comparison_variant;
  class I_vararr;
  class vararr;



  // MARK: variant

  class I_variant{
    public:
      I_variant(){};
      virtual ~I_variant(){};

      virtual int get_type() const = 0;
      constexpr static int get_static_lua_type(){return LUA_TNIL;}

      virtual void push_to_stack(const lua::api::core* lua_core) const = 0;
      
      virtual void to_string(I_string_store* pstring) const = 0;
  };

  class variant: virtual public I_variant{
    protected:
      // passed value will always be the same with the inherited type
      virtual int _compare_with(const variant* rhs) const{return 0;}

    public:
      variant();

      int get_type() const override;

      // Unused, using virtual mapping to get data from Lua might create confusion to programmers. Instead, they have to get Lua variable by independent variant constructor (this way, programmers knew what they expected). Or use lua::to_variant() to automaticaly get data from Lua without knowing what type of the data (automatically handled by the function).
      // virtual bool from_state(const lua::api::core* lua_core, int stack_index);

      void push_to_stack(const lua::api::core* lua_core) const override;
      
      // Unused, creating a copy that might use different system for allocation can introduce some confusion amongst programmers.
      // virtual variant* create_copy() const;

      void to_string(I_string_store* pstring) const override;
      virtual std::string to_string() const;

      // the lhs is itself
      int compare_with(const variant* rhs) const;
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

      virtual bool from_state(const lua::api::core* lua_core, int stack_idx) = 0;

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
      void _copy_from_var(const I_string_var* var);

      void _put_delimiter_at_end();

      void _resize_mem(std::size_t newlen);

      void _set_to_cstr(const char* cstr, std::size_t strlen);
      void _append_cstr(const char* cstr, std::size_t strlen);

      static int _strcmp(const string_var& var1, const string_var& var2);
      

    protected:
      int _compare_with(const variant* rhs) const override;

    public:
      string_var();
      string_var(const string_var& var);
      string_var(const I_string_var* var);
      string_var(const char* cstr);
      string_var(const char* cstr, std::size_t len);
      string_var(const std::string& str);
      string_var(const lua::api::core* lua_core, int stack_idx);
      ~string_var();

      int get_type() const override;

      bool from_state(const lua::api::core* lua_core, int stack_idx) override;
      void push_to_stack(const lua::api::core* lua_core) const override;

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

      virtual bool from_state(const lua::api::core* lua_core, int stack_idx) = 0;

      virtual double get_number() const = 0;
      virtual void set_number(double num) = 0;
  };

  class number_var: public variant, public I_number_var{
    private:
      lua_Number _this_data;

      void _copy_from_var(const I_number_var* var);

      static int _comp_num(const number_var& var1, const number_var& var2);

    protected:
      int _compare_with(const variant* rhs) const override;

    public:
      number_var();
      number_var(const number_var& var);
      number_var(const I_number_var* var);
      number_var(const double& from);
      number_var(const lua::api::core* lua_core, int stack_idx);

      int get_type() const override;
      
      bool from_state(const lua::api::core* lua_core, int stack_idx) override;
      void push_to_stack(const lua::api::core* lua_core) const override;

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

      virtual bool from_state(const lua::api::core* lua_core, int stack_idx) = 0;

      virtual bool get_boolean() const = 0;
      virtual void set_boolean(bool b) = 0;
  };

  class bool_var: public variant, public I_bool_var{
    private:
      bool _this_data;

      void _copy_from_var(const I_bool_var* data);

      static int _comp_bool(const bool_var& var1, const bool_var& var2);

    protected:
      int _compare_with(const variant* rhs) const override;

    public:
      bool_var();
      bool_var(const bool_var& var);
      bool_var(const I_bool_var* var);
      bool_var(const bool& from);
      bool_var(const lua::api::core* lua_core, int stack_idx);

      int get_type() const override;
      
      bool from_state(const lua::api::core* lua_core, int stack_idx) override;
      void push_to_stack(const lua::api::core* lua_core) const override;

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

      virtual bool from_state(const lua::api::core* lua_core, int stack_idx) = 0;
      virtual bool from_state_copy(const lua::api::core* lua_core, int stack_idx) = 0;
      virtual void push_to_stack_copy(const lua::api::core* lua_core) const = 0;

      // Returns NULL-terminated array.
      virtual const I_variant** get_keys() const = 0;
      virtual void update_keys() = 0;

      // This will create a new variant. Returns NULL if cannot find value.
      virtual I_variant* get_value(const I_variant* key) = 0;
      // This will create a new variant. Returns NULL if cannot find value.
      virtual const I_variant* get_value(const I_variant* key) const = 0;

      virtual void set_value(const I_variant* key, const I_variant* data) = 0;

      virtual bool remove_value(const I_variant* key) = 0;

      // Remove table values.
      virtual void clear_table() = 0;

      virtual void as_copy() = 0;

      virtual bool is_reference() const = 0;
      virtual const void* get_table_pointer() const = 0;
      virtual const lua::api::core* get_lua_core() const = 0;
  };

  // Can parse Global table, but it will skip "_G" value
  class table_var: public variant, public I_table_var{
    private:
      const I_variant** _keys_buffer;

      bool _is_ref = true;

      std::map<comparison_variant, variant*>* _table_data = NULL;

      value_ref* _tref = NULL;
      const void* _table_pointer = NULL;
      lua::api::core _lc;

      void _init_class();
      void _init_table_data();
      // Clear data (copy)
      void _clear_table_data();
      // Clear data (reference)
      void _clear_table_reference_data();
      // Clear table storage (reference and copy)
      void _clear_table();

      void _copy_from_var(const I_table_var* data);

      static std::string _get_reference_key(const void* pointer);

      // used for creating copy from lua_State
      static void _fs_iter_cb(const lua::api::core* lua_context, int key_stack, int val_stack, int iter_idx, void* cb_data);

      variant* _get_value_by_interface(const I_variant* key) const;
      void _set_value_by_interface(const I_variant* key, const I_variant* value);
      bool _remove_value_by_interface(const I_variant* key);

      variant* _get_value(const comparison_variant& comp_var) const;
      variant* _get_value(const I_variant* key) const;
      void _set_value(const comparison_variant& comp_key, const variant* value);
      // If the state of the object is not using a copy, it will create a new copy first
      void _set_value(const I_variant* key, const I_variant* value);
      bool _remove_value(const comparison_variant& comp_key);
      bool _remove_value(const I_variant* key);

      // Create memory for key registry.
      void _init_keys_reg();
      void _update_keys_reg();
      // NOTE: only clears key datas, not deallocating memory used for key registry.
      void _clear_keys_reg();

    public:
      // Default: Copy mode
      table_var();
      table_var(const table_var& var);
      table_var(const I_table_var* var);
      table_var(const lua::api::core* lua_core, int stack_idx);
      ~table_var();

      int get_type() const override;

      bool from_state(const lua::api::core* lua_core, int stack_idx) override;
      bool from_state_copy(const lua::api::core* lua_core, int stack_idx) override;
      void push_to_stack(const lua::api::core* lua_core) const override;
      void push_to_stack_copy(const lua::api::core* lua_core) const override;

      void to_string(I_string_store* pstring) const override;
      std::string to_string() const override;

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

      void clear_table() override;

      void as_copy() override;

      bool is_reference() const override;
      const void* get_table_pointer() const override;
      const lua::api::core* get_lua_core() const override;
  };



  // MARK: lightuser_var

  class I_lightuser_var: virtual public I_variant{
    public:
      constexpr static int get_static_lua_type(){return LUA_TLIGHTUSERDATA;}
      
      virtual bool from_state(const lua::api::core* lua_core, int stack_idx) = 0;
      
      virtual void* get_data() const = 0;
      virtual void set_data(void* data) = 0;
  };

  class lightuser_var: public variant, public I_lightuser_var{
    private:
      void* _pointer_data = NULL;

      void _copy_from_var(const I_lightuser_var* data);

      static int _comp_lud(const lightuser_var& var1, const lightuser_var& var2);

    protected:
      int _compare_with(const variant* rhs) const override;

    public:
      lightuser_var();
      lightuser_var(const lightuser_var& data);
      lightuser_var(const I_lightuser_var* data);
      lightuser_var(void* pointer);
      lightuser_var(const lua::api::core* lua_core, int stack_idx);

      int get_type() const override;

      bool from_state(const lua::api::core* lua_core, int stack_idx) override;
      void push_to_stack(const lua::api::core* lua_core) const override;

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
      typedef int (*luaapi_cfunction)(const lua::api::core* lua_core);

      constexpr static int get_static_lua_type(){return LUA_TFUNCTION;}

      virtual bool from_state(const lua::api::core* lua_core, int stack_idx) = 0;
      virtual bool from_state_copy(const lua::api::core* lua_core, int stack_idx) = 0;
      virtual void push_to_stack_copy(const lua::api::core* lua_core) const = 0;

      // Might return NULL if the object is a reference or not a C function.
      virtual I_vararr* get_arg_closure() = 0;
      // Might return NULL if the object is a reference or not a C function.
      virtual const I_vararr* get_arg_closure() const = 0;

      // Might return NULL if the object is a reference or not a C function.
      virtual luaapi_cfunction get_function() const = 0;
      // Might return NULL if the object is a reference or not a Lua function.
      virtual const void* get_function_binary_chunk() const = 0;
      // Might return 0 if the object is a reference or not a Lua function.
      virtual size_t get_function_binary_chunk_len() const = 0;
      // Might return NULL if the object is a reference or not a Lua function.
      virtual const char* get_function_binary_chunk_name() const = 0;
      // Might return NULL if the object is not a reference.
      virtual const void* get_lua_function_pointer() const = 0;

      // Function will not replace the C function if the object is a reference or not a C function.
      virtual bool set_function(luaapi_cfunction fn) = 0;

      // Reset to use C function
      virtual void reset_cfunction(luaapi_cfunction fn, const I_vararr* closure = NULL) = 0;
      virtual void reset_luafunction(const void* chunk_buf, size_t len, const char* chunk_name = NULL) = 0;

      virtual bool is_cfunction() const = 0;
      virtual bool is_luafunction() const = 0;
      virtual bool is_reference() const = 0;

      virtual int run_function(const lua::api::core* lua_core, const I_vararr* args, I_vararr* results) const = 0;

      virtual void as_copy() = 0;

      virtual const lua::api::core* get_lua_core() const = 0;
  };

  // NOTE: for now, only cfunction that can be stored
  // ANOTHER NOTE: any function called via function_var can use upvalues but only after LUA_FUNCVAR_START_UPVALUE
  class function_var: public I_function_var, public variant{
    private:
      luaapi_cfunction _this_fn = NULL;
      vararr* _fn_args = NULL;

      // for Lua
      void* _fn_binary_data = NULL;
      size_t _fn_binary_len = 0;
      char* _fn_binary_name = NULL;

      lua::api::core _lc;
      const void* _func_pointer = NULL;
      value_ref* _fref = NULL;

      bool _is_reference = false;
      bool _is_cfunction = true;

      void _init_class();
      void _init_cfunction();
      void _clear_function_data();

      void _copy_from_var(const I_function_var* data);

      static std::string _get_reference_key(const void* pointer);
    
      // Function definition will be ignored if specified that Lua code does not exist in this case
      static int _cb_entry_point(lua_State* state);

      // Function definition will be ignored if specified that Lua code does not exist in this case
      static int _fn_chunk_data_writer(lua_State* state, const void* p, size_t sz, void* ud);
      // Function definition will be ignored if specified that Lua code does not exist in this case
      static const char* _fn_chunk_data_reader(lua_State* state, void* data, size_t* size);

    protected:
      // compared results are based on the function pointer
      int _compare_with(const variant* rhs) const override;

    public:
      // Default: Copy mode
      function_var();
      function_var(const function_var& var);
      function_var(const I_function_var* var);
      function_var(luaapi_cfunction fn, const I_vararr* closure = NULL);
      function_var(const void* chunk_buf, size_t len, const char* chunk_name = NULL);
      function_var(const lua::api::core* lua_core, int stack_idx);
      ~function_var();

      int get_type() const override;
      
      bool from_state(const lua::api::core* lua_core, int stack_idx) override;
      bool from_state_copy(const lua::api::core* lua_core, int stack_idx) override;
      // NOTE: pushing to stack with bound CFunction as NULL, it will push a nil value.
      void push_to_stack(const lua::api::core* lua_core) const override;
      void push_to_stack_copy(const lua::api::core* lua_core) const override;

      std::string to_string() const;
      void to_string(I_string_store* pstring) const override;

      I_vararr* get_arg_closure() override;
      const I_vararr* get_arg_closure() const override;

      luaapi_cfunction get_function() const override;
      const void* get_function_binary_chunk() const override;
      size_t get_function_binary_chunk_len() const override;
      const char* get_function_binary_chunk_name() const override;
      const void* get_lua_function_pointer() const override;

      // NOTE: any function called via function_var can use upvalues but only after LUA_FUNCVAR_START_UPVALUE
      bool set_function(luaapi_cfunction fn) override;

      void reset_cfunction(luaapi_cfunction fn, const I_vararr* closure = NULL) override;
      void reset_luafunction(const void* chunk_buf, size_t len, const char* chunk_name = NULL) override;

      bool is_cfunction() const override;
      bool is_luafunction() const override;
      bool is_reference() const override;

      int run_function(const lua::api::core* lua_core, const I_vararr* args, I_vararr* results) const override;

      void as_copy() override;

      const lua::api::core* get_lua_core() const override;
  };




  // MARK: error_var

  class I_error_var: virtual public I_variant{
    public:
      constexpr static int get_static_lua_type(){return LUA_TERROR;}

      virtual bool from_state(const lua::api::core* lua_core, int stack_idx) = 0;

      virtual const I_variant* get_error_data_interface() const = 0;
      virtual long long get_error_code() const = 0;
  };

  class error_var: public variant, public I_error_var{
    private:
      variant* _err_data = NULL;
      long long _error_code = -1;

      void _copy_from_var(const I_error_var* data);

      void _clear_object();

    public:
      error_var();
      error_var(const error_var& var);
      error_var(const I_error_var* var);
      error_var(const variant* data, long long code);
      error_var(const I_variant* data, long long code);
      error_var(const lua::api::core* lua_core, int stack_idx);
      ~error_var();

      int get_type() const override;

      bool from_state(const lua::api::core* lua_core, int stack_idx) override;
      void push_to_stack(const lua::api::core* lua_core) const override;

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

  // Once the object registered within object_var, the object lifetime (memory) management shouldn't be handled by user code. That management will be handled by Lua's garbage collector.
  // Object to store C++ object inside Lua codespace. Note, that this object will not create a clone when pushing same object. Instead, the object will push a table referencing the object.
  // NOTE: Actual representation in Lua is a table with metamethods
  class I_object_var: virtual public I_variant{
    public:
      constexpr static int get_static_lua_type(){return LUA_TCPPOBJECT;}

      virtual I_object* get_object_reference() const = 0;

      virtual bool object_pushed_to_lua() const = 0;

      virtual const lua::api::core* get_lua_core() const = 0;
  };

  class object_var: public I_object_var, public variant{
    private:
      lua::api::core _lc;

      I_object* _obj = NULL;
      value_ref* _oref = NULL;

      void _copy_from_var(const I_object_var* data);
      void _clear_object();

      static std::string _get_reference_key(void* idata);

    public:
      object_var(const object_var& data);
      object_var(const I_object_var* data);
      object_var(const lua::api::core* lua_core, I_object* obj);
      object_var(const lua::api::core* lua_core, int stack_idx);
      ~object_var();
      
      int get_type() const override;

      bool from_state(const lua::api::core* lua_core, int stack_idx);
      bool from_object_reference(const lua::api::core* lua_core, I_object* obj);
      // If object is NULL then pushed nil.
      void push_to_stack(const lua::api::core* lua_core) const override;

      void to_string(I_string_store* pstring) const override;
      std::string to_string() const override;
      
      I_object* get_object_reference() const override;
      
      bool object_pushed_to_lua() const override;

      const lua::api::core* get_lua_core() const override;
  };




  // MARK: comparison_variant

  class comparison_variant{
    private:
      variant* _this_var;

      void _init_class(const I_variant* from);

    public:
      comparison_variant(const I_variant* from);
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
}



// MARK: Static functions

lua::variant* cpplua_create_var_copy(const lua::I_variant* data);



// MARK: DLL functions

#define CPPLUA_VARIANT_SET_DEFAULT_LOGGER cpplua_variant_set_default_logger
#define CPPLUA_VARIANT_SET_DEFAULT_LOGGER_STR MACRO_TO_STR_EXP(CPPLUA_VARIANT_SET_DEFAULT_LOGGER)

#define CPPLUA_DELETE_VARIANT cpplua_delete_variant
#define CPPLUA_DELETE_VARIANT_STR MACRO_TO_STR_EXP(CPPLUA_DELETE_VARIANT)

typedef void (__stdcall *var_set_def_logger_func)(I_logger* logger);
typedef void (__stdcall *del_var_func)(const lua::I_variant* data);

DLLEXPORT void CPPLUA_VARIANT_SET_DEFAULT_LOGGER(I_logger* logger);
DLLEXPORT void CPPLUA_DELETE_VARIANT(const lua::I_variant* data);


#endif