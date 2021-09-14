#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include "config.h"
#include "SDCard.h"
#include "Epub.h"
#include "RubbishHtmlParser/RubbishHtmlParser.h"
#include "Renderer/EpdRenderer.h"
#include "Renderer/ConsoleRenderer.h"

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
  // read the epub file
  // Epub *epub = new Epub("/sdcard/pg2701.epub");
  Epub *epub = new Epub("/sdcard/pg14838-images.epub");
  ESP_LOGI("main", "After epub create: %d", esp_get_free_heap_size());
  // get the current section contents
  epub->next_section();
  ESP_LOGI("main", "After skip section 1: %d", esp_get_free_heap_size());
  // epub->next_section();
  // ESP_LOGI("main", "After skip section 2: %d", esp_get_free_heap_size());
  const char *html = epub->get_section_contents(epub->get_current_section());
  ESP_LOGI("main", "After read html: %d", esp_get_free_heap_size());
  // parse the html
  RubbishHtmlParser *parser = new RubbishHtmlParser(html, strlen(html));
  ESP_LOGI("main", "After parse: %d", esp_get_free_heap_size());
  parser->layout(renderer);
  ESP_LOGI("main", "After layout: %d", esp_get_free_heap_size());
  for (int page = 0; page < parser->get_page_count(); page++)
  {
    ESP_LOGI("main", "rendering page %d of %d", page, parser->get_page_count());
    renderer->clear_display();
    parser->render_page(page, renderer);
    ESP_LOGI("main", "rendered page %d of %d", page, parser->get_page_count());
    // log out the free memory
    ESP_LOGI("main", "after render: %d", esp_get_free_heap_size());
    vTaskDelay(pdMS_TO_TICKS(2000));
    renderer->flush_display();
  }
  esp_deep_sleep_start();

  // create the console renderer
  // ConsoleRenderer *renderer = new ConsoleRenderer();
  // // initialise the SDCard
  // SDCard *sdcard = new SDCard("/sdcard", PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK, PIN_NUM_CS);
  // // read the epub file
  // Epub *epub = new Epub("/sdcard/pg2701.epub");
  // // Epub *epub = new Epub("/sdcard/pg14838-images.epub");

  // // get the current section contents
  // epub->next_section();
  // const char *html = epub->get_section_contents(epub->get_current_section());
  // // ESP_LOGI("main", "html: %s", html);
  // // parse the html
  // RubbishHtmlParser *parser = new RubbishHtmlParser(html, strlen(html));
  // ESP_LOGI("main", "parsed html");
  // parser->layout(renderer);
  // ESP_LOGI("main", "layed out html");
  // for (int page = 0; page < parser->get_page_count(); page++)
  // {
  //   ESP_LOGI("main", "rendering page %d of %d", page, parser->get_page_count());

  //   parser->render_page(page, renderer);
  //   ESP_LOGI("main", "rendered page %d of %d", page, parser->get_page_count());
  //   vTaskDelay(pdMS_TO_TICKS(2000));
  // }
  esp_deep_sleep_start();
}

void app_main()
{
  xTaskCreate(main_task, "main_task", 16384, NULL, 1, NULL);
}