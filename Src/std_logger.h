#ifndef STD_LOGGER_HEADER
#define STD_LOGGER_HEADER

#include "I_logger.h"


// This code will be statically bind to the compilation file

class std_logger: public I_logger{
  public:
    void print(std::string message) const;
    void print_warning(std::string message) const;
    void print_error(std::string message) const;

    void print(const char* message) const override;
    void print_warning(const char* message) const override;
    void print_error(const char* message) const override;
};


I_logger* get_std_logger();


#endif