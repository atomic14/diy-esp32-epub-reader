#include <string.h>
#ifndef UNIT_TEST
#include <esp_log.h>
#include <esp_system.h>
#else
#define ESP_LOGI(args...)
#define ESP_LOGE(args...)
#define ESP_LOGI(args...)
#define ESP_LOGD(args...)
#endif
#include "EpubReader.h"
#include "Epub.h"
#include "../RubbishHtmlParser/RubbishHtmlParser.h"
#include "../Renderer/Renderer.h"

static const char *TAG = "EREADER";

bool EpubReader::load()
{
  ESP_LOGD(TAG, "Before epub load: %d", esp_get_free_heap_size());
  // do we need to load the epub?
  if (!epub || epub->get_path() != state.epub_path)
  {
    renderer->show_busy();
    delete epub;
    delete parser;
    parser = nullptr;
    epub = new Epub(state.epub_path);
    if (epub->load())
    {
      ESP_LOGD(TAG, "After epub load: %d", esp_get_free_heap_size());
      return false;
    }
  }
  return true;
}

void EpubReader::parse_and_layout_current_section()
{
  if (!parser)
  {
    renderer->show_busy();
    ESP_LOGD(TAG, "Parse and render section %d", state.current_section);
    ESP_LOGD(TAG, "Before read html: %d", esp_get_free_heap_size());
    std::string item = epub->get_spine_item(state.current_section);
    std::string base_path = item.substr(0, item.find_last_of('/') + 1);
    char *html = reinterpret_cast<char *>(epub->get_item_contents(item));
    ESP_LOGD(TAG, "After read html: %d", esp_get_free_heap_size());
    parser = new RubbishHtmlParser(html, strlen(html), base_path);
    free(html);
    ESP_LOGD(TAG, "After parse: %d", esp_get_free_heap_size());
    parser->layout(renderer, epub);
    ESP_LOGD(TAG, "After layout: %d", esp_get_free_heap_size());
    state.pages_in_current_section = parser->get_page_count();
  }
}

void EpubReader::next()
{
  state.current_page++;
  if (state.current_page >= state.pages_in_current_section)
  {
    state.current_section++;
    state.current_page = 0;
    delete parser;
    parser = nullptr;
  }
}

void EpubReader::prev()
{
  if (state.current_page == 0)
  {
    if (state.current_section > 0)
    {
      delete parser;
      parser = nullptr;
      state.current_section--;
      ESP_LOGD(TAG, "Going to previous section %d", state.current_section);
      parse_and_layout_current_section();
      state.current_page = state.pages_in_current_section - 1;
      return;
    }
  }
  state.current_page--;
}

void EpubReader::render()
{
  if (!parser)
  {
    parse_and_layout_current_section();
  }
  ESP_LOGD(TAG, "rendering page %d of %d", state.current_page, parser->get_page_count());
  parser->render_page(state.current_page, renderer, epub);
  ESP_LOGD(TAG, "rendered page %d of %d", state.current_page, parser->get_page_count());
  ESP_LOGD(TAG, "after render: %d", esp_get_free_heap_size());
}
