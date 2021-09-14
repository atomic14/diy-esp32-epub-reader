#pragma once

#include <esp_log.h>

#include "tinyxml2.h"
#include "ZipFile.h"
#include <vector>
#include <map>

#define TAG "EPUB"

class Epub
{
private:
  const char *m_path = nullptr;
  int m_current_section = 0;
  std::vector<std::string> m_sections;

public:
  Epub(const char *path)
  {
    m_path = path;
    // read the manifest and spine
    // the spine gives us the order of the files
    // the manifest gives us the names of the files
    // we can then read the files in the order they are in the spine
    ESP_LOGI(TAG, "Epub: %s", m_path);
    ZipFile zip(m_path);
    const char *contents = (const char *)zip.read_file_to_memory("OEBPS/content.opf");
    ESP_LOGI(TAG, "contents: %s", contents);
    // parse the contents
    tinyxml2::XMLDocument doc;
    auto result = doc.Parse(contents);
    if (result != tinyxml2::XML_SUCCESS)
    {
      ESP_LOGE(TAG, "Error parsing content.opf - %s", doc.ErrorIDToName(result));
    }
    // find the manifest
    auto manifest = doc.FirstChildElement("package")->FirstChildElement("manifest");
    if (!manifest)
    {
      ESP_LOGE(TAG, "Missing manifest");
    }
    // create a mapping from id to file name
    auto item = manifest->FirstChildElement("item");
    std::map<std::string, std::string> items;
    while (item)
    {
      items[item->Attribute("id")] = std::string("OEBPS/") + item->Attribute("href");
      item = item->NextSiblingElement("item");
    }
    // find the spine
    auto spine = doc.FirstChildElement("package")->FirstChildElement("spine");
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
    // dump out the spint
    for (auto &s : m_sections)
    {
      ESP_LOGI(TAG, "Spine: %s", s.c_str());
    }
  }
  int get_current_section()
  {
    return m_current_section;
  }
  void set_current_section(int section)
  {
    m_current_section = section;
  }
  const char *get_section_contents(int section)
  {
    if (section < 0 || section >= m_sections.size())
    {
      ESP_LOGI(TAG, "Invalid section %d", section);
      return nullptr;
    }
    ZipFile zip(m_path);
    ESP_LOGI(TAG, "Loading Section: %s", m_sections[section].c_str());
    auto content = (const char *)zip.read_file_to_memory(m_sections[section].c_str());
    if (!content)
    {
      ESP_LOGE(TAG, "Failed to read section");
      return nullptr;
    }
    return content;
  }
  bool next_section()
  {
    if (m_current_section + 1 >= m_sections.size())
    {
      return false;
    }
    m_current_section++;
    return true;
  }
  ~Epub()
  {
  }
};