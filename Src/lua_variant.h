#ifndef LUA_VARIANT_HEADER
#define LUA_VARIANT_HEADER

#include "I_logger.h"
#include "lua_includes.h"

#include "map"
#include "string"


#define LUA_TERROR 0x101


// TODO maybe add more pointer based var, but with only ref based

namespace lua{
  void set_default_logger(I_logger* logger); 

  class comparison_variant;
  class variant{
    protected:
      // passed value will always be the same with the inherited type
      virtual int _compare_with(const variant* rhs) const{return 0;}

    public:
      variant();
      virtual ~variant(){}

      virtual int get_type() const;

      constexpr static int get_static_lua_type(){return LUA_TNIL;}

      virtual bool from_state(lua_State* state, int stack_index){return false;}
      virtual void push_to_stack(lua_State* state) const {lua_pushnil(state);}
      virtual variant* create_copy() const {return new variant();}

      virtual std::string to_string() const {return "nil";}

      // the lhs is itself
      int compare_with(const variant* rhs) const;

      void setglobal(lua_State* state, std::string var_name);
      void setglobal(lua_State* state, const char* var_name);
  };


  class nil_var: public variant{};


  class string_var: public variant{
    private:
      char* _str_mem = NULL;
      size_t _mem_size = 0;

      void _init_class();

      void _put_delimiter_at_end();

      void _resize_mem(size_t newlen);

      void _set_to_cstr(const char* cstr, size_t strlen);
      void _append_cstr(const char* cstr, size_t strlen);

      static int _strcmp(const string_var& var1, const string_var& var2);

    protected:
      int _compare_with(const variant* rhs) const override;

    public:
      string_var();
      string_var(const string_var& var1);
      string_var(const char* cstr);
      string_var(const char* cstr, size_t len);
      string_var(const std::string& str);
      string_var(lua_State* state, int stack_idx);
      ~string_var();

      int get_type() const override;

      constexpr static int get_static_lua_type(){return LUA_TSTRING;}

      bool from_state(lua_State* state, int stack_idx) override;
      void push_to_stack(lua_State* state) const override;
      variant* create_copy() const override;

      std::string to_string() const override;

      string_var& operator+=(const string_var& var1);
      string_var& operator+=(const std::string& var1);
      string_var& operator+=(const char* var1);

      string_var operator+(const string_var& var1) const;
      string_var operator+(const std::string& var1) const;
      string_var operator+(const char* var1) const;

      const char& operator[](size_t i) const;

      bool operator<(const string_var& var1) const;
      bool operator>(const string_var& var1) const;
      bool operator<=(const string_var& var1) const;
      bool operator>=(const string_var& var1) const;
      bool operator==(const string_var& var1) const;
      
      void append(const string_var& str);
      void append(const std::string& str);
      void append(const char* cstr);
      void append(const char* cstr, size_t len);

      const char* get_str_data() const;
      size_t get_str_len() const;
  };


  class number_var: public variant{
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

      constexpr static int get_static_lua_type(){return LUA_TNUMBER;}
      
      bool from_state(lua_State* state, int stack_idx) override;
      void push_to_stack(lua_State* state) const override;
      variant* create_copy() const override;

      std::string to_string() const override;

      double get_number() const;

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

  class bool_var: public variant{
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

      constexpr static int get_static_lua_type(){return LUA_TBOOLEAN;}
      
      bool from_state(lua_State* state, int stack_idx) override;
      void push_to_stack(lua_State* state) const override;
      variant* create_copy() const override;

      std::string to_string() const override;

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


  // Can parse Global table, but it will skip "_G" value
  class table_var: public variant{
    private:
      std::map<comparison_variant, variant*> _table_data;

      void _init_class();

      void _copy_from_var(const table_var& var);

      void _clear_table_data();

      static void _fs_iter_cb(lua_State* state, int key_stack, int val_stack, int iter_idx, void* cb_data);

      variant* _get_value(const comparison_variant& comp_var) const;

    public:
      table_var();
      table_var(const table_var& var);
      table_var(lua_State* state, int stack_idx);
      ~table_var();

      int get_type() const override;

      constexpr static int get_static_lua_type(){return LUA_TTABLE;}

      bool from_state(lua_State* state, int stack_idx) override;
      void push_to_stack(lua_State* state) const override;
      variant* create_copy() const override;

      std::string to_string() const override;

      variant* get_value(const comparison_variant& comp_var);
      const variant* get_value(const comparison_variant& comp_var) const;

      void set_value(const comparison_variant& comp_var, variant* value);

      bool remove_value(const comparison_variant& comp_var);
  };


  class lightuser_var: public variant{
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

      constexpr static int get_static_lua_type(){return LUA_TLIGHTUSERDATA;}

      bool from_state(lua_State* state, int stack_idx) override;
      void push_to_stack(lua_State* state) const override;
      variant* create_copy() const override;

      std::string to_string() const override;

      const void* get_data() const;
      void* get_data();


      bool operator==(const lightuser_var& var1) const;
      bool operator!=(const lightuser_var& var1) const;
      bool operator<(const lightuser_var& var1) const;
      bool operator>(const lightuser_var& var1) const;
      bool operator<=(const lightuser_var& var1) const;
      bool operator>=(const lightuser_var& var1) const; 
  };



  class error_var: public variant{
    private:
      variant* _err_data = NULL;
      int _error_code = -1;

    public:
      error_var();
      error_var(const error_var& var);
      error_var(lua_State* state, int stack_idx);

      int get_type() const override;

      constexpr static int get_static_lua_type(){return LUA_TERROR;}

      bool from_state(lua_State* state, int stack_idx) override;
      void push_to_stack(lua_State* state) const override;
      variant* create_copy() const override;

      std::string to_string() const override;

      const variant* get_error_data() const;
      int get_error_code() const;

      void set_error_code(int error_code);
  };



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



  variant* to_variant(lua_State* state, int stack_idx);
  variant* to_variant_fglobal(lua_State* state, const char* global_name);
  variant* to_variant_ref(lua_State* state, int stack_idx);

  string_var from_pointer_to_str(void* pointer);
  void* from_str_to_pointer(const string_var& str);
}

#endif