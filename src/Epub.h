#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <esp_log.h>

#include "tinyxml2.h"
#include "ZipFile.h"
#include <vector>
#include <map>

#define TAG "EPUB"

class Epub
{
private:
  // the title read from the EPUB meta data
  std::string m_title;
  // the path in the zip file to the cover image
  std::string m_cover_archive_path;
  // the path on the sdcard where the cover has been written
  std::string m_cover_path;
  // where is the EPUBfile?
  std::string m_path;
  // folder to store the extracted files
  std::string m_extract_path;
  // the sections of the EPUB file
  std::vector<std::string> m_sections;

public:
  Epub(const char *path)
  {
    m_path = path;
    size_t lastindex = m_path.find_last_of(".");
    m_extract_path = m_path.substr(0, lastindex);
    // create the extract path if it does not exist
    struct stat m_file_info;
    if (stat(m_extract_path.c_str(), &m_file_info) == -1)
    {
      mkdir(m_extract_path.c_str(), 0700);
    }
  }
  // load in the meta data for the epub file
  bool load()
  {
    ZipFile zip(m_path.c_str());
    const char *contents = (const char *)zip.read_file_to_memory("OEBPS/content.opf");
    // parse the contents
    tinyxml2::XMLDocument doc;
    auto result = doc.Parse(contents);
    if (result != tinyxml2::XML_SUCCESS)
    {
      ESP_LOGE(TAG, "Error parsing content.opf - %s", doc.ErrorIDToName(result));
    }
    auto package = doc.FirstChildElement("package");
    // get the metadata - title and cover image
    auto metadata = package->FirstChildElement("metadata");
    if (!metadata)
    {
      ESP_LOGE(TAG, "Missing metadata");
      return false;
    }
    auto title = metadata->FirstChildElement("dc:title");
    if (!title)
    {
      ESP_LOGE(TAG, "Missing title");
      return false;
    }
    m_title = title->GetText();
    auto cover = metadata->FirstChildElement("meta");
    if (!cover)
    {
      ESP_LOGE(TAG, "Missing cover");
      return false;
    }
    auto cover_item = cover->Attribute("content");
    // read the manifest and spine
    // the manifest gives us the names of the files
    // the spine gives us the order of the files
    // we can then read the files in the order they are in the spine
    auto manifest = package->FirstChildElement("manifest");
    if (!manifest)
    {
      ESP_LOGE(TAG, "Missing manifest");
    }
    // create a mapping from id to file name
    auto item = manifest->FirstChildElement("item");
    std::map<std::string, std::string> items;
    while (item)
    {
      std::string item_id = item->Attribute("id");
      std::string href = item->Attribute("href");
      // grab the cover image
      if (item_id == cover_item)
      {
        m_cover_path = href;
      }
      items[item_id] = href;
      item = item->NextSiblingElement("item");
    }
    // find the spine
    auto spine = package->FirstChildElement("spine");
    if (!spine)
    {
      ESP_LOGE(TAG, "Missing spine");
    }
    // read the spine
    auto itemref = spine->FirstChildElement("itemref");
    while (itemref)
    {
      auto id = itemref->Attribute("idref");
      if (items.find(id) != items.end())
      {
        m_sections.push_back(items[id]);
      }
      itemref = itemref->NextSiblingElement("itemref");
    }
    // dump out the spine
    for (auto &s : m_sections)
    {
      ESP_LOGD(TAG, "Spine: %s", s.c_str());
    }
    return true;
  }
  std::string get_image_path(const std::string &image_name)
  {
    // check to see if the file exists
    std::string dst_file = m_extract_path + "/" + image_name;
    FILE *fp = fopen(dst_file.c_str(), "rb");
    if (!fp)
    {
      fclose(fp);
      ESP_LOGI(TAG, "Extracting image: %s", image_name.c_str());
      std::string archive_path = "OEBPS/" + image_name;
      ZipFile zip(m_path.c_str());
      bool res = zip.read_file_to_file(archive_path.c_str(), dst_file.c_str());
      if (!res)
      {
        ESP_LOGE(TAG, "Failed to extract image: %s", image_name.c_str());
      }
    }
    else
    {
      fclose(fp);
      ESP_LOGI(TAG, "Image already extracted: %s", image_name.c_str());
    }
    return dst_file;
  }
  int get_sections_count()
  {
    return m_sections.size();
  }
  char *get_section_contents(int section)
  {
    if (section < 0 || section >= m_sections.size())
    {
      ESP_LOGI(TAG, "Invalid section %d", section);
      return nullptr;
    }
    ZipFile zip(m_path.c_str());
    ESP_LOGI(TAG, "Loading Section: %s", m_sections[section].c_str());
    std::string archive_path = "OEBPS/" + m_sections[section];
    auto content = (char *)zip.read_file_to_memory(archive_path.c_str());
    if (!content)
    {
      ESP_LOGE(TAG, "Failed to read section");
      return nullptr;
    }
    return content;
  }
  ~Epub()
  {
  }
};