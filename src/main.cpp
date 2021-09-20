#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/rtc_io.h>
#include <driver/gpio.h>
#include <esp_sleep.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <esp32/ulp.h>
#include "config.h"
#include "SDCard.h"
#include "EpubList/Epub.h"
#include "EpubList/EpubList.h"
#include "EpubList/EpubReader.h"
#include <RubbishHtmlParser/RubbishHtmlParser.h>
#include "Renderer/EpdRenderer.h"
#include "Renderer/ConsoleRenderer.h"
#include <list>
#include <string.h>
#include <regular_font.h>
#include <bold_font.h>
#include <italic_font.h>
#include <bold_italic_font.h>
#include "ulp_main.h"

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");

extern "C"
{
  void app_main();
}

const gpio_num_t BUTTON_UP_GPIO_NUM = GPIO_NUM_34;
const gpio_num_t BUTTON_DOWN_GPIO_NUM = GPIO_NUM_39;
const gpio_num_t BUTTON_SELECT_GPIO_NUM = GPIO_NUM_35;

const char *TAG = "main";

typedef enum
{
  SELECTING_EPUB,
  READING_EPUB
} UIState;

typedef struct
{
  // what mode are we currently in?
  UIState ui_state;
  // the currently selected epub
  int epub_index;
  // the filename of the epub we are currently reading
  char epub_filename[256];
  // the section we are currently reading
  int current_section;
  // the page we are currently reading
  int page;
} STATE;

// default to showing the list of epubs to the user
RTC_DATA_ATTR UIState ui_state = SELECTING_EPUB;
// the state data for the epub list
RTC_DATA_ATTR EpubListState epub_list_state;
// the sate data for reading an epub
RTC_DATA_ATTR EpubReaderState epub_reader_state;

typedef enum
{
  NONE,
  UP,
  DOWN,
  SELECT
} UIAction;

void handleEpubList(Renderer *renderer, UIAction action);
void handleEpub(Renderer *renderer, UIAction action);

static void init_ulp_program()
{
  esp_err_t err = ulp_load_binary(0, ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
  ESP_ERROR_CHECK(err);

  gpio_num_t gpio_p1 = GPIO_NUM_34;
  gpio_num_t gpio_p2 = GPIO_NUM_39;
  gpio_num_t gpio_p3 = GPIO_NUM_35;

  //    GPIO 34 - RTC 4
  //    GPIO 39 - RTC 3
  //    GPIO 35 - RTC 5

  rtc_gpio_init(gpio_p1);
  rtc_gpio_set_direction(gpio_p1, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pulldown_dis(gpio_p1);
  rtc_gpio_pullup_dis(gpio_p1);
  rtc_gpio_hold_en(gpio_p1);
  // rtc_gpio_isolate(GPIO_NUM_34);

  rtc_gpio_init(gpio_p2);
  rtc_gpio_set_direction(gpio_p2, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pulldown_dis(gpio_p2);
  rtc_gpio_pullup_dis(gpio_p2);
  rtc_gpio_hold_en(gpio_p2);
  // rtc_gpio_isolate(GPIO_NUM_39);

  rtc_gpio_init(gpio_p3);
  rtc_gpio_set_direction(gpio_p3, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pulldown_dis(gpio_p3);
  rtc_gpio_pullup_dis(gpio_p3);
  rtc_gpio_hold_en(gpio_p3);
  // rtc_gpio_isolate(GPIO_NUM_35);

  ulp_set_wakeup_period(0, 100 * 1000); // 100 ms
  err = ulp_run(&ulp_entry - RTC_SLOW_MEM);

  ESP_ERROR_CHECK(err);
}

void handleEpub(Renderer *renderer, UIAction action)
{
  EpubReader *reader = new EpubReader(epub_reader_state, renderer);
  switch (action)
  {
  case UP:
    reader->next();
    break;
  case DOWN:
    reader->prev();
    break;
  case SELECT:
    // switch back to main screen
    ui_state = SELECTING_EPUB;
    handleEpubList(renderer, NONE);
    break;
  case NONE:
  default:
    break;
  }
  reader->render();
  renderer->flush_display();
}

void handleEpubList(Renderer *renderer, UIAction action)
{
  // load up the epub list from the filesystem
  ESP_LOGI("main", "Loading epub files");
  EpubList *epubList = new EpubList(epub_list_state);
  if (epubList->load("/sdcard/"))
  {
    ESP_LOGI("main", "Epub files loaded");
  }
  // work out what the user wante us to do
  switch (action)
  {
  case UP:
    epubList->next();
    break;
  case DOWN:
    epubList->prev();
    break;
  case SELECT:
    strcpy(epub_reader_state.epub_path, epubList->get_current_epub_path());
    epub_reader_state.current_section = 0;
    epub_reader_state.current_page = 0;
    ui_state = READING_EPUB;
    handleEpub(renderer, NONE);
    break;
  case NONE:
  default:
    // nothing to do
    break;
  }
  ESP_LOGI("main", "previous_rendered_page=%d", epub_list_state.previous_rendered_page);
  ESP_LOGI("main", "previous_selected_item=%d", epub_list_state.previous_selected_item);
  ESP_LOGI("main", "selected_item=%d", epub_list_state.selected_item);
  epubList->render(renderer);
  renderer->flush_display();
}

void setup_pin(gpio_num_t gpio_num)
{
  if (!rtc_gpio_is_valid_gpio(gpio_num))
  {
    ESP_LOGE(TAG, "GPIO %d cannot be used in deep sleep", gpio_num);
  }
  gpio_pad_select_gpio(gpio_num);
  gpio_set_direction(gpio_num, GPIO_MODE_INPUT);
  rtc_gpio_hold_en(gpio_num);
}

void main_task(void *param)
{
  ESP_LOGI("main", "Memory before renderer init: %d", esp_get_free_heap_size());
  // create the EPD renderer
  EpdRenderer *renderer = new EpdRenderer(&regular_font, &bold_font, &italic_font, &bold_italic_font);
  ESP_LOGI("main", "Memory after renderer init: %d", esp_get_free_heap_size());
  // initialise the SDCard
  SDCard *sdcard = new SDCard("/sdcard", PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK, PIN_NUM_CS);
  ESP_LOGI("main", "Memory after sdcard init: %d", esp_get_free_heap_size());

  // work out if we were woken via EXT1 and which button was pressed
  UIAction ui_action = NONE;
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1)
  {
    ESP_LOGI("main", "Woken by EXT1");
    // work out which button was pressed
    uint64_t button_pressed = esp_sleep_get_ext1_wakeup_status();
    // if (button_pressed & (1ULL << BUTTON_UP_GPIO_NUM))
    // {
    //   ESP_LOGI("main", "Button UP pressed");
    //   ui_action = UP;
    // }
    if (button_pressed & (1ULL << BUTTON_DOWN_GPIO_NUM))
    {
      ESP_LOGI("main", "Button DOWN pressed");
      ui_action = DOWN;
    }
    else if (button_pressed & (1ULL << BUTTON_SELECT_GPIO_NUM))
    {
      ESP_LOGI("main", "Button SELECT pressed");
      ui_action = SELECT;
    }
    else
    {
      ESP_LOGI("main", "Unknown button pressed %lld", button_pressed);
    }
  }
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_ULP)
  {
    ESP_LOGI("main", "Woken by ULP %d", ulp_gpio_status);
    if (ulp_gpio_status & (1 << rtc_io_number_get(BUTTON_UP_GPIO_NUM)))
    {
      ui_action = UP;
    }
    if (ulp_gpio_status & (1 << rtc_io_number_get(BUTTON_DOWN_GPIO_NUM)))
    {
      ui_action = DOWN;
    }
    if (ulp_gpio_status & (1 << rtc_io_number_get(BUTTON_SELECT_GPIO_NUM)))
    {
      ui_action = SELECT;
    }
  }

  switch (ui_state)
  {
  case READING_EPUB:
    handleEpub(renderer, ui_action);
    break;
  case SELECTING_EPUB:
  default:
    handleEpubList(renderer, ui_action);
    break;
  }
  // setup the buttons for deep sleep
  // setup_pin(BUTTON_UP_GPIO_NUM);
  // setup_pin(BUTTON_DOWN_GPIO_NUM);
  // setup_pin(BUTTON_SELECT_GPIO_NUM);
  // esp_sleep_enable_ext1_wakeup(
  //     // (1ULL << BUTTON_UP_GPIO_NUM) |
  //     (1ULL << BUTTON_DOWN_GPIO_NUM) | (1ULL << BUTTON_SELECT_GPIO_NUM),
  //     ESP_EXT1_WAKEUP_ANY_HIGH);
  // esp_deep_sleep_start();

  // // read the epub file
  // // Epub *epub = new Epub("/sdcard/pg2701.epub");
  // Epub *epub = new Epub("/sdcard/pg14838-images.epub");
  // // Epub *epub = new Epub("/sdcard/pg19337-images.epub");
  // epub->load();
  // ESP_LOGI("main", "After epub create: %d", esp_get_free_heap_size());
  // // go through the book
  // for (int section_idx = 0; section_idx < epub->get_sections_count(); section_idx++)
  // {
  //   char *html = epub->get_section_contents(section_idx);
  //   ESP_LOGI("main", "After read html: %d", esp_get_free_heap_size());
  //   RubbishHtmlParser *parser = new RubbishHtmlParser(html, strlen(html));
  //   ESP_LOGI("main", "After parse: %d", esp_get_free_heap_size());
  //   parser->layout(renderer, epub);
  //   ESP_LOGI("main", "After layout: %d", esp_get_free_heap_size());
  //   for (int page = 0; page < parser->get_page_count(); page++)
  //   {
  //     ESP_LOGI("main", "rendering page %d of %d", page, parser->get_page_count());
  //     parser->render_page(page, renderer);
  //     ESP_LOGI("main", "rendered page %d of %d", page, parser->get_page_count());
  //     // log out the free memory
  //     ESP_LOGI("main", "after render: %d", esp_get_free_heap_size());
  //     renderer->flush_display();
  //     vTaskDelay(pdMS_TO_TICKS(1000));
  //   }
  //   delete parser;
  //   delete html;
  // }
  //
  init_ulp_program();
  ESP_LOGI("main", "Entering deep sleep");
  ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());
  esp_deep_sleep_start();
}

void app_main()
{
  ESP_LOGI("main", "Memory before main task start %d", esp_get_free_heap_size());
  xTaskCreatePinnedToCore(main_task, "main_task", 32768, NULL, 1, NULL, 1);
}