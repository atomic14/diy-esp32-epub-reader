#pragma once

#include <string>
#include <vector>
#include <unordered_map>

class ZipFile;

class Epub
{
private:
  // the title read from the EPUB meta data
  std::string m_title;
  // the cover image
  std::string m_cover_image_item;
  // where is the EPUBfile?
  std::string m_path;
  // the path of the *.ncx index
  std::string toc_path;
  // the base path for items in the EPUB file
  std::string m_base_path;
  // the spine of the EPUB file
  std::vector<std::pair<std::string, std::string>> m_spine;
  // find the path for the content.opf file
  bool find_content_opf_file(ZipFile &zip, std::string &content_opf_file);
  std::string get_toc_filename();

public:
  Epub(const std::string &path);
  ~Epub() {}
  std::string &get_base_path() { return m_base_path; }
  bool load();
  // Fills toc_index map
  bool loadIndex();
  // Finds the src value in the index
  std::string get_index_src(int index);
  int get_index_size();

  const std::string &get_path() const { return m_path; }
  const std::string &get_title();
  const std::string &get_cover_image_item();
  uint8_t *get_item_contents(const std::string &item_href, size_t *size = nullptr);
  std::string &get_spine_item(int spine_index);
  int get_spine_item_id(std::string spine_key);
  int get_spine_items_count();

  // is public since we will use it from EpubIndex
  std::vector<std::string> split(std::string to_split, std::string delimiter);

  // the index of chapters
  std::vector<std::pair<std::string, std::string>> toc_index;
};