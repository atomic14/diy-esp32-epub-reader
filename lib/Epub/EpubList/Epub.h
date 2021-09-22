#pragma once

#include <string>
#include <vector>

class Epub
{
private:
  // the title read from the EPUB meta data
  std::string m_title;
  // the cover image
  std::string m_cover_image_name;
  // where is the EPUBfile?
  std::string m_path;
  // folder to store the extracted files (we pull out the images to disk)
  std::string m_extract_path;
  // the spine of the EPUB file
  std::vector<std::string> m_spine;

public:
  Epub(const std::string &path);
  ~Epub() {}
  bool load();
  const std::string &get_path() const { return m_path; }
  const std::string &get_title();
  const std::string get_cover_image_filename();
  std::string get_image_path(const std::string &image_name);
  int get_spine_items_count();
  char *get_spine_item_contents(int section);
};