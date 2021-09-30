#pragma once

#include <string>

class SPIFFS
{
private:
  std::string m_mount_point;
public:
  SPIFFS(const char *mount_point);
  ~SPIFFS();
  const std::string &get_mount_point() { return m_mount_point; }
};