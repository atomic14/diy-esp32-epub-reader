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
#include <epd_driver.h>

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");

extern "C"
{
  void app_main();
}

const char *TAG = "main";

typedef enum
{
  SELECTING_EPUB,
  READING_EPUB
} UIState;

// default to showing the list of epubs to the user
RTC_DATA_ATTR UIState ui_state = SELECTING_EPUB;
// the state data for the epub list
RTC_DATA_ATTR EpubListState epub_list_state;
// the sate data for reading an epub
RTC_DATA_ATTR EpubReaderState epub_reader_state;

// the pins for the buttons
const gpio_num_t BUTTON_UP_GPIO_NUM = GPIO_NUM_34; // RTC 4
const gpio_num_t BUTTON_DOWN_GPIO_NUM = GPIO_NUM_39; // RTC 3
const gpio_num_t BUTTON_SELECT_GPIO_NUM = GPIO_NUM_35; // RTC 5

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

  gpio_num_t gpio_p1 = BUTTON_UP_GPIO_NUM;
  gpio_num_t gpio_p2 = BUTTON_DOWN_GPIO_NUM;
  gpio_num_t gpio_p3 = BUTTON_SELECT_GPIO_NUM;

  rtc_gpio_init(gpio_p1);
  rtc_gpio_set_direction(gpio_p1, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pulldown_dis(gpio_p1);
  rtc_gpio_pullup_dis(gpio_p1);
  rtc_gpio_hold_dis(gpio_p1);

  rtc_gpio_init(gpio_p2);
  rtc_gpio_set_direction(gpio_p2, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pulldown_dis(gpio_p2);
  rtc_gpio_pullup_dis(gpio_p2);
  rtc_gpio_hold_dis(gpio_p2);

  rtc_gpio_init(gpio_p3);
  rtc_gpio_set_direction(gpio_p3, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pulldown_dis(gpio_p3);
  rtc_gpio_pullup_dis(gpio_p3);
  rtc_gpio_hold_dis(gpio_p3);

  ulp_set_wakeup_period(0, 100 * 1000); // 100 ms
  err = ulp_run(&ulp_entry - RTC_SLOW_MEM);

  ESP_ERROR_CHECK(err);
}

EpubReader *reader = nullptr;
void handleEpub(Renderer *renderer, UIAction action)
{
  if (!reader)
  {
    reader = new EpubReader(epub_reader_state, renderer);
  }
  reader->load();
  switch (action)
  {
  case UP:
    reader->prev();
    break;
  case DOWN:
    reader->next();
    break;
  case SELECT:
    // switch back to main screen
    ui_state = SELECTING_EPUB;
    renderer->clear_screen();
    // force a redraw
    epub_list_state.previous_rendered_page = -1;
    handleEpubList(renderer, NONE);
    return;
  case NONE:
  default:
    break;
  }
  reader->render();
  renderer->flush_display(true);
}

static EpubList *epubList = nullptr;
void handleEpubList(Renderer *renderer, UIAction action)
{
  // load up the epub list from the filesystem
  if (!epubList)
  {
    ESP_LOGI("main", "Loading epub files");
    epubList = new EpubList(epub_list_state);
    if (epubList->load("/sdcard/"))
    {
      ESP_LOGI("main", "Epub files loaded");
    }
  }
  // work out what the user wante us to do
  switch (action)
  {
  case UP:
    epubList->prev();
    break;
  case DOWN:
    epubList->next();
    break;
  case SELECT:
    strcpy(epub_reader_state.epub_path, epubList->get_current_epub_path());
    epub_reader_state.current_section = 0;
    epub_reader_state.current_page = 0;
    ui_state = READING_EPUB;
    renderer->clear_screen();
    renderer->flush_display();
    handleEpub(renderer, NONE);
    return;
  case NONE:
  default:
    // nothing to do
    break;
  }
  epubList->render(renderer);
}

void handleUserInteraction(Renderer *renderer, UIAction ui_action)
{
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
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_ULP)
  {
    // restore the renderer state - it should have been saved when we went to sleep...
    renderer->hydrate();

    uint16_t rtc_pin = ulp_gpio_status & UINT16_MAX;
    ESP_LOGI("main", "Woken by ULP %d", rtc_pin);
    if (rtc_pin == rtc_io_number_get(BUTTON_UP_GPIO_NUM))
    {
      ESP_LOGI("main", "Button UP pressed");
      handleUserInteraction(renderer, UP);
    }
    else if (rtc_pin == rtc_io_number_get(BUTTON_DOWN_GPIO_NUM))
    {
      ESP_LOGI("main", "Button DOWN pressed");
      handleUserInteraction(renderer, DOWN);
    }
    else if (rtc_pin == rtc_io_number_get(BUTTON_SELECT_GPIO_NUM))
    {
      ESP_LOGI("main", "Button SELECT pressed");
      handleUserInteraction(renderer, SELECT);
    }
    else
    {
      handleUserInteraction(renderer, NONE);
    }
  }
  else
  {
    ESP_LOGI(TAG, "Initialise ULP program");
    init_ulp_program();

    renderer->clear_display();
    handleUserInteraction(renderer, NONE);
  }

  gpio_set_direction(BUTTON_UP_GPIO_NUM, GPIO_MODE_INPUT);
  gpio_set_direction(BUTTON_DOWN_GPIO_NUM, GPIO_MODE_INPUT);
  gpio_set_direction(BUTTON_SELECT_GPIO_NUM, GPIO_MODE_INPUT);

  int64_t last_user_interaction = esp_timer_get_time();
  while (esp_timer_get_time() - last_user_interaction < 30 * 1000 * 1000)
  {
    // check to see if up is presses
    if (gpio_get_level(BUTTON_DOWN_GPIO_NUM) == 0)
    {
      handleUserInteraction(renderer, DOWN);
      last_user_interaction = esp_timer_get_time();
    }
    else if (gpio_get_level(BUTTON_UP_GPIO_NUM) == 0)
    {
      handleUserInteraction(renderer, UP);
      last_user_interaction = esp_timer_get_time();
    }
    else if (gpio_get_level(BUTTON_SELECT_GPIO_NUM) == 0)
    {
      handleUserInteraction(renderer, SELECT);
      last_user_interaction = esp_timer_get_time();
    }
    else
    {
      vTaskDelay(pdMS_TO_TICKS(50));
    }
  }

  ESP_LOGI("main", "Saving state");
  // save the state
  renderer->dehydrate();

  ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());
  ESP_LOGI("main", "Entering deep sleep");
  esp_deep_sleep_start();
}

void app_main()
{
  ESP_LOGI("main", "Memory before main task start %d", esp_get_free_heap_size());
  xTaskCreatePinnedToCore(main_task, "main_task", 32768, NULL, 1, NULL, 1);
}