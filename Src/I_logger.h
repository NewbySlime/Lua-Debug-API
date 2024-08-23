#ifndef I_LOGGER_HEADER
#define I_LOGGER_HEADER


#include "string"


class I_logger{
  public:
    virtual void print(std::string message) const = 0;
    virtual void print_warning(std::string message) const = 0;
    virtual void print_error(std::string message) const = 0;

    virtual void print(const char* message) const = 0;
    virtual void print_warning(const char* message) const = 0;
    virtual void print_error(const char* message) const = 0;
};

#endif