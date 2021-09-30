#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifndef UNIT_TEST
#include <esp_log.h>
#else
#define ESP_LOGI(args...)
#define ESP_LOGE(args...)
#define ESP_LOGD(args...)
#endif
#include <map>
#include "tinyxml2.h"
#include "../ZipFile/ZipFile.h"
#include "Epub.h"

static const char *TAG = "EPUB";

Epub::Epub(const std::string &path) : m_path(path)
{
}

// load in the meta data for the epub file
bool Epub::load()
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
    return false;
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
      m_cover_image_item = href;
    }
    items[item_id] = href;
    item = item->NextSiblingElement("item");
  }
  // find the spine
  auto spine = package->FirstChildElement("spine");
  if (!spine)
  {
    ESP_LOGE(TAG, "Missing spine");
    return false;
  }
  // read the spine
  auto itemref = spine->FirstChildElement("itemref");
  while (itemref)
  {
    auto id = itemref->Attribute("idref");
    if (items.find(id) != items.end())
    {
      m_spine.push_back(items[id]);
    }
    itemref = itemref->NextSiblingElement("itemref");
  }
  return true;
}

const std::string &Epub::get_title()
{
  return m_title;
}

const std::string &Epub::get_cover_image_item()
{
  return m_cover_image_item;
}

int Epub::get_spine_items_count()
{
  return m_spine.size();
}

uint8_t *Epub::get_item_contents(const std::string &item_href, size_t *size)
{
  ZipFile zip(m_path.c_str());
  std::string archive_path = std::string("OEBPS/") + item_href;
  auto content = zip.read_file_to_memory(archive_path.c_str(), size);
  if (!content)
  {
    ESP_LOGE(TAG, "Failed to read section");
    return nullptr;
  }
  return content;
}

char *Epub::get_spine_item_contents(int spine_index)
{
  if (spine_index < 0 || spine_index >= m_spine.size())
  {
    ESP_LOGE(TAG, "Invalid spine_index %d", spine_index);
    return nullptr;
  }
  ESP_LOGD(TAG, "Loading Section: %s", m_spine[spine_index].c_str());
  return (char *)get_item_contents(m_spine[spine_index].c_str());
}
