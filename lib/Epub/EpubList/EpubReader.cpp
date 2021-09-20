#include <string.h>
#include <esp_log.h>
#include <esp_system.h>
#include "EpubReader.h"
#include "Epub.h"
#include "../RubbishHtmlParser/RubbishHtmlParser.h"

static const char *TAG = "EREADER";

bool EpubReader::load()
{
  ESP_LOGI(TAG, "Before epub load: %d", esp_get_free_heap_size());
  epub = new Epub(state.epub_path);
  if (epub->load())
  {
    ESP_LOGI(TAG, "After epub load: %d", esp_get_free_heap_size());
    return false;
  }
  return true;
}

void EpubReader::parse_and_layout_current_section()
{
  ESP_LOGI(TAG, "BEfore read html: %d", esp_get_free_heap_size());
  char *html = epub->get_section_contents(state.current_section);
  ESP_LOGI(TAG, "After read html: %d", esp_get_free_heap_size());
  parser = new RubbishHtmlParser(html, strlen(html));
  ESP_LOGI(TAG, "After parse: %d", esp_get_free_heap_size());
  parser->layout(renderer, epub);
  state.pages_in_current_section = parser->get_page_count();
}

void EpubReader::next()
{
  state.current_page++;
  if (state.current_page >= state.pages_in_current_section)
  {
    state.current_section++;
    state.current_page = 0;
  }
  parse_and_layout_current_section();
}

void EpubReader::prev()
{
  if (state.current_page == 0)
  {
    if (state.current_section > 0)
    {
      state.current_section--;
      parse_and_layout_current_section();
      state.current_page = state.pages_in_current_section - 1;
      return;
    }
  }
  state.current_page--;
  parse_and_layout_current_section();
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
}
