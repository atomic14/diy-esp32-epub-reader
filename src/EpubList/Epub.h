#pragma once

#include <string>
#include <vector>

class Epub
{
private:
  // the title read from the EPUB meta data
  std::string m_title;
  // the path in the zip file to the cover image
  std::string m_cover_archive_path;
  // where is the EPUBfile?
  std::string m_path;
  // folder to store the extracted files
  std::string m_extract_path;
  // the sections of the EPUB file
  std::vector<std::string> m_sections;

public:
  Epub(const std::string &path);
  ~Epub() {}
  bool load();
  const std::string &get_title();
  const std::string get_cover_image_filename();
  std::string get_image_path(const std::string &image_name);
  int get_sections_count();
  char *get_section_contents(int section);
};