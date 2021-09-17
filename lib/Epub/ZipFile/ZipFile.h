#pragma once

#include <string>

class ZipFile
{
private:
  std::string m_filename;

public:
  ZipFile(const char *filename)
  {
    m_filename = filename;
  }
  ~ZipFile() {}
  // read a file from the zip file allocating the required memory for the data
  const char *read_file_to_memory(const char *filename);
  bool read_file_to_file(const char *filename, const char *dest);
};