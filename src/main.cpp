#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include "config.h"
#include "SDCard.h"
#include "Epub.h"
#include "RubbishHtmlParser.h"

extern "C"
{
  void app_main();
}

void main_task(void *param)
{
  // initialise the SDCard
  SDCard *sdcard = new SDCard("/sdcard", PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK, PIN_NUM_CS);
  // read the epub file
  //  Epub *epub = new Epub("/sdcard/pg2701.epub");
  Epub *epub = new Epub("/sdcard/pg14838-images.epub");

  // get the current section contents
  epub->next_section();
  const char *html = epub->get_section_contents(epub->get_current_section());
  // ESP_LOGI("main", "html: %s", html);
  // parse the html
  RubbishHtmlParser *parser = new RubbishHtmlParser(html, strlen(html));

  esp_deep_sleep_start();
}

void app_main()
{
  xTaskCreate(main_task, "main_task", 16384, NULL, 1, NULL);
}