#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include "config.h"
#include "SDCard.h"
#include "EpubList/Epub.h"
#include "EpubList/EpubList.h"
#include "RubbishHtmlParser/RubbishHtmlParser.h"
#include "Renderer/EpdRenderer.h"
#include "Renderer/ConsoleRenderer.h"
#include <list>
#include <string.h>

extern "C"
{
  void app_main();
}

void main_task(void *param)
{
  ESP_LOGI("main", "Memory before renderer init: %d", esp_get_free_heap_size());
  // create the EPD renderer
  EpdRenderer *renderer = new EpdRenderer();
  ESP_LOGI("main", "Memory after renderer init: %d", esp_get_free_heap_size());
  // initialise the SDCard
  SDCard *sdcard = new SDCard("/sdcard", PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK, PIN_NUM_CS);
  ESP_LOGI("main", "Memory after sdcard init: %d", esp_get_free_heap_size());
  ESP_LOGI("main", "Loading epub files");
  EpubList *epubList = new EpubList();
  if (epubList->load("/sdcard/"))
  {
    ESP_LOGI("main", "Epub files loaded");
    for (int i = 0; i < epubList->get_num_epubs(); i++)
    {
      epubList->render(i, renderer);
      renderer->flush_display();
      vTaskDelay(1);
    }
  }
  else
  {
    ESP_LOGE("main", "Epub files not loaded");
  }
  esp_deep_sleep_start();

  // read the epub file
  // Epub *epub = new Epub("/sdcard/pg2701.epub");
  Epub *epub = new Epub("/sdcard/pg14838-images.epub");
  // Epub *epub = new Epub("/sdcard/pg19337-images.epub");
  epub->load();
  ESP_LOGI("main", "After epub create: %d", esp_get_free_heap_size());
  // go through the book
  for (int section_idx = 0; section_idx < epub->get_sections_count(); section_idx++)
  {
    char *html = epub->get_section_contents(section_idx);
    ESP_LOGI("main", "After read html: %d", esp_get_free_heap_size());
    RubbishHtmlParser *parser = new RubbishHtmlParser(html, strlen(html));
    ESP_LOGI("main", "After parse: %d", esp_get_free_heap_size());
    parser->layout(renderer, epub);
    ESP_LOGI("main", "After layout: %d", esp_get_free_heap_size());
    for (int page = 0; page < parser->get_page_count(); page++)
    {
      ESP_LOGI("main", "rendering page %d of %d", page, parser->get_page_count());
      parser->render_page(page, renderer);
      ESP_LOGI("main", "rendered page %d of %d", page, parser->get_page_count());
      // log out the free memory
      ESP_LOGI("main", "after render: %d", esp_get_free_heap_size());
      renderer->flush_display();
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
    delete parser;
    delete html;
  }
  esp_deep_sleep_start();
}

void app_main()
{
  ESP_LOGI("main", "Memory before main task start %d", esp_get_free_heap_size());
  xTaskCreatePinnedToCore(main_task, "main_task", 32768, NULL, 1, NULL, 1);
}