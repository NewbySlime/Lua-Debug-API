#ifndef STRING_STORE_HEADER
#define STRING_STORE_HEADER

#include "string"


// This code will be statically bind to the compilation file
// If a code returns an interface (I_xx) create a copy with using statically linked compilation function if the code that returns comes from dynamic library

/// @brief Interface class for string_store class.
class I_string_store{
  public:
    virtual void clear() = 0;
    virtual void append(const char* data) = 0;
    virtual void append(const char* data, std::size_t length) = 0;
};

/// @brief Helper class for storing a std::string that is specific to current compilation, with functions redirected by interface functions.
class string_store: public I_string_store{
  public:
    string_store(){}
    string_store(const char* cstr);
    string_store(const std::string& str);

    std::string data;

    void clear() override;
    void append(const char* data) override;
    void append(const char* data, std::size_t length) override;
};

#endif