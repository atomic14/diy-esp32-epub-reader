#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_sleep.h>
#include "config.h"
#ifndef USE_SPIFFS
#include "SDCard.h"
#else
#include "SPIFFS.h"
#endif
#include "EpubList/Epub.h"
#include "EpubList/EpubList.h"
#include "EpubList/EpubReader.h"
#include <RubbishHtmlParser/RubbishHtmlParser.h>
#ifdef USE_EPD_DISPLAY
#include "Renderer/EpdiyRenderer.h"
#endif
#ifdef USE_M5PAPER_DISPLAY
#include "Renderer/M5PaperRenderer.h"
#endif
#include <regular_font.h>
#include <bold_font.h>
#include <italic_font.h>
#include <bold_italic_font.h>
#include <hourglass.h>
#include "Renderer/ConsoleRenderer.h"
#include "controls/ButtonControls.h"
#ifdef USE_TOUCH
#include "controls/TouchControls.h"
#endif
#ifdef BATTERY_ADC_CHANNEL
#include "battery/Battery.h"
static Battery *battery = nullptr;
#endif

#ifdef LOG_ENABLED
// Reference: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html
#define LOG_LEVEL ESP_LOG_INFO
#else
#define LOG_LEVEL ESP_LOG_NONE
#endif
#include <esp_log.h>
// The SD Card shares the same GPIO pins as the touch controller so you must use SPIFFS
#ifdef USE_TOUCH
#ifndef USE_SPIFFS
#error "USE_SPIFFS must be defined if USE_TOUCH is defined"
#endif
#endif

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
RTC_NOINIT_ATTR UIState ui_state = SELECTING_EPUB;
// the state data for the epub list and reader
RTC_DATA_ATTR EpubListState epub_list_state;

void handleEpub(Renderer *renderer, UIAction action);
void handleEpubList(Renderer *renderer, UIAction action, bool needs_redraw);

static EpubList *epub_list = nullptr;
static EpubReader *reader = nullptr;

void handleEpub(Renderer *renderer, UIAction action)
{
  if (!reader)
  {
    reader = new EpubReader(epub_list_state.epub_list[epub_list_state.selected_item], renderer);
    reader->load();
  }
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
    // clear the epub reader away
    delete reader;
    reader = nullptr;
    // force a redraw
    if (!epub_list)
    {
      epub_list = new EpubList(renderer, &epub_list_state);
    }
    handleEpubList(renderer, NONE, true);
    return;
  case NONE:
  default:
    break;
  }
  reader->render();
}

void handleEpubList(Renderer *renderer, UIAction action, bool needs_redraw)
{
  // load up the epub list from the filesystem
  if (!epub_list)
  {
    ESP_LOGI("main", "Creating epub list");
    epub_list = new EpubList(renderer, &epub_list_state);
    if (epub_list->load("/fs/"))
    {
      ESP_LOGI("main", "Epub files loaded");
    }
  }
  if (needs_redraw)
  {
    epub_list->set_needs_redraw();
  }
  // work out what the user wants us to do
  switch (action)
  {
  case UP:
    epub_list->prev();
    break;
  case DOWN:
    epub_list->next();
    break;
  case SELECT:
    // switch to reading the epub
    // setup the reader state
    ui_state = READING_EPUB;
    // create the reader and load the book
    reader = new EpubReader(epub_list_state.epub_list[epub_list_state.selected_item], renderer);
    reader->load();
    handleEpub(renderer, NONE);
    return;
  case NONE:
  default:
    // nothing to do
    break;
  }
  epub_list->render();
}

void handleUserInteraction(Renderer *renderer, UIAction ui_action, bool needs_redraw)
{
  switch (ui_state)
  {
  case READING_EPUB:
    handleEpub(renderer, ui_action);
    break;
  case SELECTING_EPUB:
  default:
    handleEpubList(renderer, ui_action, needs_redraw);
    break;
  }
}

// TODO - add the battery level
void draw_battery_level(Renderer *renderer, float voltage, float percentage)
{
  // clear the margin so we can draw the battery in the right place
  renderer->set_margin_top(0);
  int width = 40;
  int height = 20;
  int margin_right = 5;
  int margin_top = 10;
  int xpos = renderer->get_page_width() - width - margin_right;
  int ypos = margin_top;
  int percent_width = width * percentage / 100;
  renderer->fill_rect(xpos, ypos, width, height, 255);
  renderer->fill_rect(xpos + width - percent_width, ypos, percent_width, height, 0);
  renderer->draw_rect(xpos, ypos, width, height, 0);
  renderer->fill_rect(xpos - 4, ypos + height / 4, 4, height / 2, 0);
  // put the margin back
  renderer->set_margin_top(35);
}

void main_task(void *param)
{
#ifndef USE_M5PAPER_DISPLAY
#ifdef CONFIG_EPD_BOARD_REVISION_LILYGO_T5_47
  // Need to power on the EDP to get power to the SD Card (Only in Lilygo model)
  // Not when using EPDiy since first epd_init() has to be called to initialize stuff
  epd_poweron();
#endif
#endif

  // create the EPD renderer
#ifdef USE_M5PAPER_DISPLAY
  Renderer *renderer = new M5PaperRenderer(
      &regular_font,
      &bold_font,
      &italic_font,
      &bold_italic_font,
      hourglass_data,
      hourglass_width,
      hourglass_height);
#else
  Renderer *renderer = new EpdiyRenderer(
      &regular_font,
      &bold_font,
      &italic_font,
      &bold_italic_font,
      hourglass_data,
      hourglass_width,
      hourglass_height);
#endif
#ifdef USE_SPIFFS
  ESP_LOGI("main", "Using SPIFFS");
  // create the file system
  SPIFFS *spiffs = new SPIFFS("/fs");
#else
  ESP_LOGI("main", "Using SDCard");
  // initialise the SDCard
  SDCard *sdcard = new SDCard("/fs", SD_CARD_PIN_NUM_MISO, SD_CARD_PIN_NUM_MOSI, SD_CARD_PIN_NUM_CLK, SD_CARD_PIN_NUM_CS);
#endif
  // dump out the epub list state
  ESP_LOGI("main", "epub list state num_epubs=%d", epub_list_state.num_epubs);
  ESP_LOGI("main", "epub list state is_loaded=%d", epub_list_state.is_loaded);
  ESP_LOGI("main", "epub list state selected_item=%d", epub_list_state.selected_item);

#ifdef BATTERY_ADC_CHANNEL
  battery = new Battery(BATTERY_ADC_CHANNEL);
  ESP_LOGI("main", "Battery %.0f, %.2fv", battery->get_percentage(), battery->get_voltage());
  ESP_LOGI("main", "Memory before renderer init: %d", esp_get_free_heap_size());
#endif

  // make space for the battery
  renderer->set_margin_top(35);
  // page margins
  renderer->set_margin_left(10);
  renderer->set_margin_right(10);

  ESP_LOGI("main", "Memory after renderer init: %d", esp_get_free_heap_size());
  ESP_LOGI("main", "Memory after FS init: %d", esp_get_free_heap_size());
  // create a message queue for UI events
  xQueueHandle ui_queue = xQueueCreate(10, sizeof(UIAction));

  // set the controls up
  ESP_LOGI("main", "Setting up controls");
  ButtonControls *controls = new ButtonControls(
      BUTTON_UP_GPIO_NUM,
      BUTTON_DOWN_GPIO_NUM,
      BUTTON_SELECT_GPIO_NUM,
      BUTONS_ACTIVE_LEVEL,
      [ui_queue](UIAction action)
      {
        xQueueSend(ui_queue, &action, 0);
      });

#ifdef USE_TOUCH
  TouchControls *touch_controls = new TouchControls(
      renderer,
      EPD_WIDTH,
      EPD_HEIGHT,
      3,
      [ui_queue](UIAction action)
      {
        xQueueSend(ui_queue, &action, 0);
      });
#endif

  ESP_LOGI("main", "Controls configured");
  // work out if we were woken from deep sleep
  if (controls->did_wake_from_deep_sleep())
  {
    // restore the renderer state - it should have been saved when we went to sleep...
    bool hydrate_success = renderer->hydrate();
    UIAction ui_action = controls->get_deep_sleep_action();
    handleUserInteraction(renderer, ui_action, !hydrate_success);
  }
  else
  {
    // reset the screen
    renderer->reset();
    // make sure the UI is in the right state
    handleUserInteraction(renderer, NONE, true);
  }

#ifdef BATTERY_ADC_CHANNEL
  // draw the battery level before flushing the screen
  draw_battery_level(renderer, battery->get_voltage(), battery->get_percentage());
#endif

#ifdef USE_TOUCH
  touch_controls->render(renderer);
#endif
  renderer->flush_display();

  // keep track of when the user last interacted and go to sleep after N seconds
  int64_t last_user_interaction = esp_timer_get_time();
  while (esp_timer_get_time() - last_user_interaction < 120 * 1000 * 1000)
  {
    UIAction ui_action = NONE;
    // wait for something to happen for 60 seconds
    if (xQueueReceive(ui_queue, &ui_action, pdMS_TO_TICKS(60000)) == pdTRUE)
    {
      if (ui_action != NONE)
      {
        // something happened!
        last_user_interaction = esp_timer_get_time();

#ifdef USE_TOUCH
        // show feedback on the touch controls
        touch_controls->renderPressedState(renderer, ui_action);
#endif
        handleUserInteraction(renderer, ui_action, false);

#ifdef USE_TOUCH
        // make sure to clear the feedback on the touch controls
        touch_controls->render(renderer);
#endif
      }
    }
// update the battery level - do this even if there is no interaction so we
// show the battery level even if the user is idle
#ifdef BATTERY_ADC_CHANNEL
    draw_battery_level(renderer, battery->get_voltage(), battery->get_percentage());
#endif
    renderer->flush_display();
  }
  ESP_LOGI("main", "Saving state");
  // save the state of the renderer
  renderer->dehydrate();
  // turn off the SD Card
#ifdef USE_SPIFFS
  delete spiffs;
#else
// seems to cause issues with the M5 Paper
#ifndef USE_M5PAPER_DISPLAY
  delete sdcard;
#endif
#endif
  ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());
  ESP_LOGI("main", "Entering deep sleep");
#ifdef USE_M5PAPER_DISPLAY
  rtc_gpio_init(GPIO_NUM_2);
  rtc_gpio_set_direction(GPIO_NUM_2, RTC_GPIO_MODE_OUTPUT_ONLY);
  rtc_gpio_set_level(GPIO_NUM_2, 1);
  rtc_gpio_hold_en(GPIO_NUM_2);
#else
  epd_poweroff();
#endif
  // configure deep sleep options
  controls->setup_deep_sleep();
  vTaskDelay(pdMS_TO_TICKS(500));
  // go to sleep
  esp_deep_sleep_start();
}

void app_main()
{
  // Logging control
  esp_log_level_set("main", LOG_LEVEL);
  esp_log_level_set("EPUB", LOG_LEVEL);
  esp_log_level_set("PUBLIST", LOG_LEVEL);
  esp_log_level_set("ZIP", LOG_LEVEL);
  esp_log_level_set("JPG", LOG_LEVEL);
  esp_log_level_set("TOUCH", LOG_LEVEL);
  ESP_LOGI("main", "Memory before main task start %d", esp_get_free_heap_size());
  xTaskCreatePinnedToCore(main_task, "main_task", 32768, NULL, 1, NULL, 1);
}