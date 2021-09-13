#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <epd_driver.h>
#include <epd_highlevel.h>
#include "config.h"
#include "SDCard.h"
#include "Epub.h"
#include "RubbishHtmlParser.h"
#include "firasans.h"

extern "C"
{
  void app_main();
}

void main_task(void *param)
{
  // initialise the SDCard
  SDCard *sdcard = new SDCard("/sdcard", PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK, PIN_NUM_CS);
  // read the epub file
  Epub *epub = new Epub("/sdcard/pg2701.epub");
  // Epub *epub = new Epub("/sdcard/pg14838-images.epub");

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
  // First setup epd to use later
  epd_init(EPD_OPTIONS_DEFAULT);
  auto hl = epd_hl_init(EPD_BUILTIN_WAVEFORM);
  epd_set_rotation(EPD_ROT_INVERTED_PORTRAIT);
  uint8_t *fb = epd_hl_get_framebuffer(&hl);
  epd_clear();
  EpdFontProperties font_props = epd_font_properties_default();
  font_props.flags = EPD_DRAW_ALIGN_LEFT;

  // first set full screen to white
  epd_hl_set_all_white(&hl);

  /************* Display the text itself ******************/
  // hardcoded to start at upper left corner
  // with bit of padding
  int cursor_x = 10;
  int cursor_y = 30;
  epd_write_string(&FiraSans, "Hello World!", &cursor_x, &cursor_y, fb, &font_props);
  epd_poweron();

  ESP_LOGI(TAG, "epd_ambient_temperature=%f", epd_ambient_temperature());

  auto err = epd_hl_update_screen(&hl, MODE_GC16, 20);
  vTaskDelay(500);
  epd_poweroff();

  xTaskCreate(main_task, "main_task", 16384, NULL, 1, NULL);
}