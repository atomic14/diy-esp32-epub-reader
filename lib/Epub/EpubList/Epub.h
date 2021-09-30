#pragma once

#include <string>
#include <vector>

class Epub
{
private:
  // the title read from the EPUB meta data
  std::string m_title;
  // the cover image
  std::string m_cover_image_item;
  // where is the EPUBfile?
  std::string m_path;
  // the spine of the EPUB file
  std::vector<std::string> m_spine;

public:
  Epub(const std::string &path);
  ~Epub() {}
  bool load();
  const std::string &get_path() const { return m_path; }
  const std::string &get_title();
  const std::string &get_cover_image_item();
  uint8_t *get_item_contents(const std::string &item_href, size_t *size = nullptr);
  char *get_spine_item_contents(int spine_index);
  int get_spine_items_count();
};