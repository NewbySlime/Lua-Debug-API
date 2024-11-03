#ifndef FILE_LOGGER_HEADER
#define FILE_LOGGER_HEADER

#include "I_logger.h"
#include "string"
#include "Windows.h"

class file_logger: public I_logger{
  private:
    HANDLE _file_handle = INVALID_HANDLE_VALUE;
    HANDLE _file_mutex = INVALID_HANDLE_VALUE;

  public:
    file_logger(const std::string& file_path, bool append_file);
    ~file_logger();

    void print(const char* message) const override;
    void print_warning(const char* message) const override;
    void print_error(const char* message) const override;

    void flush();
    void close();

    // Can also be used for checking if the file has been opened or not (at this object construction).
    bool is_closed() const;
};

#endif