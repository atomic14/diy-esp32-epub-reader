#include <string.h>
#ifndef UNIT_TEST
#include <esp_log.h>
#include <esp_system.h>
#else
#define ESP_LOGI(args...)
#define ESP_LOGE(args...)
#define ESP_LOGD(args...)
#endif
#include "EpubReader.h"
#include "Epub.h"
#include "../RubbishHtmlParser/RubbishHtmlParser.h"
#include "../Renderer/Renderer.h"

static const char *TAG = "EREADER";

bool EpubReader::load()
{
  ESP_LOGI(TAG, "Before epub load: %d", esp_get_free_heap_size());
  // do we need to load the epub?
  if (!epub || epub->get_path() != state.epub_path)
  {
    delete epub;
    delete parser;
    parser = nullptr;
    epub = new Epub(state.epub_path);
    if (epub->load())
    {
      ESP_LOGI(TAG, "After epub load: %d", esp_get_free_heap_size());
      return false;
    }
  }
  return true;
}

void EpubReader::parse_and_layout_current_section()
{
  if (!parser)
  {
    ESP_LOGI(TAG, "Parse and render section %d", state.current_section);
    ESP_LOGI(TAG, "Before read html: %d", esp_get_free_heap_size());
    char *html = epub->get_spine_item_contents(state.current_section);
    ESP_LOGI(TAG, "After read html: %d", esp_get_free_heap_size());
    parser = new RubbishHtmlParser(html, strlen(html));
    free(html);
    ESP_LOGI(TAG, "After parse: %d", esp_get_free_heap_size());
    parser->layout(renderer, epub);
    ESP_LOGI(TAG, "After layout: %d", esp_get_free_heap_size());
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
      ESP_LOGI(TAG, "Going to previous section %d", state.current_section);
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
  ESP_LOGI(TAG, "rendering page %d of %d", state.current_page, parser->get_page_count());
  parser->render_page(state.current_page, renderer);
  ESP_LOGI(TAG, "rendered page %d of %d", state.current_page, parser->get_page_count());
  ESP_LOGI(TAG, "after render: %d", esp_get_free_heap_size());
  renderer->flush_display();
}
