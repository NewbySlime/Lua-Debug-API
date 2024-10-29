#ifndef I_LOGGER_HEADER
#define I_LOGGER_HEADER


#include "string"


class I_logger{
  public:
    /* NOTE: this part of code is not valid when it is used in dynamic library context 
    virtual void print(std::string message) const = 0;
    virtual void print_warning(std::string message) const = 0;
    virtual void print_error(std::string message) const = 0;
    */

    virtual void print(const char* message) const = 0;
    virtual void print_warning(const char* message) const = 0;
    virtual void print_error(const char* message) const = 0;
};

#endif