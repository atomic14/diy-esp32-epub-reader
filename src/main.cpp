#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_sleep.h>
#include <esp_log.h>
#include "config.h"
#include "SDCard.h"
#include "EpubList/Epub.h"
#include "EpubList/EpubList.h"
#include "EpubList/EpubReader.h"
#include <RubbishHtmlParser/RubbishHtmlParser.h>
#include "Renderer/EpdRenderer.h"
#include <regular_font.h>
#include <bold_font.h>
#include <italic_font.h>
#include <bold_italic_font.h>
#include "Renderer/ConsoleRenderer.h"
#include "controls/Controls.h"

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

void handleEpub(Renderer *renderer, UIAction action);
void handleEpubList(Renderer *renderer, UIAction action);

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
    // clear the epub reader away
    delete reader;
    reader = nullptr;
    // force a redraw
    epub_list_state.previous_rendered_page = -1;
    handleEpubList(renderer, NONE);
    return;
  case NONE:
  default:
    break;
  }
  reader->render();
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
    reader = new EpubReader(epub_reader_state, renderer);

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
  // need to power on the EDP to get power to the SD Card
  epd_poweron();
  ESP_LOGI("main", "Memory before renderer init: %d", esp_get_free_heap_size());
  // create the EPD renderer
  Renderer *renderer = new EpdRenderer(&regular_font, &bold_font, &italic_font, &bold_italic_font);
  //Renderer *renderer = new ConsoleRenderer();
  ESP_LOGI("main", "Memory after renderer init: %d", esp_get_free_heap_size());
  // initialise the SDCard
  SDCard *sdcard = new SDCard("/sdcard", SD_CARD_PIN_NUM_MISO, SD_CARD_PIN_NUM_MOSI, SD_CARD_PIN_NUM_CLK, SD_CARD_PIN_NUM_CS);
  ESP_LOGI("main", "Memory after sdcard init: %d", esp_get_free_heap_size());
  // set the controls up
  Controls *controls = new Controls(BUTTON_UP_GPIO_NUM, BUTTON_DOWN_GPIO_NUM, BUTTON_SELECT_GPIO_NUM, 0);

  // work out if we were woken from deep sleep
  if (controls->did_wake_from_deep_sleep())
  {
    // restore the renderer state - it should have been saved when we went to sleep...
    renderer->hydrate();

    UIAction ui_action = controls->get_deep_sleep_action();
    handleUserInteraction(renderer, ui_action);
  }
  else
  {
    // reset the screen
    renderer->reset();
    // make sure the UI is in the right state
    handleUserInteraction(renderer, NONE);
  }
  // configure the button inputs
  controls->setup_inputs();
  // keep track of when the user last interacted and go to sleep after 30 seconds
  int64_t last_user_interaction = esp_timer_get_time();
  while (esp_timer_get_time() - last_user_interaction < 30 * 1000 * 1000)
  {
    // check for user interaction
    UIAction ui_action = controls->get_action();
    if (ui_action != NONE)
    {
      last_user_interaction = esp_timer_get_time();
      handleUserInteraction(renderer, ui_action);
    }
    else
    {
      // wait for the user to do something
      vTaskDelay(pdMS_TO_TICKS(50));
    }
  }

  ESP_LOGI("main", "Saving state");
  // save the state
  renderer->dehydrate();
  // turn off the SD Card
  delete sdcard;
  ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());
  ESP_LOGI("main", "Entering deep sleep");
  epd_poweroff();
  // configure deep sleep options
  controls->setup_deep_sleep();
  // go to sleep
  esp_deep_sleep_start();
}

void app_main()
{
  ESP_LOGI("main", "Memory before main task start %d", esp_get_free_heap_size());
  xTaskCreatePinnedToCore(main_task, "main_task", 32768, NULL, 1, NULL, 1);
}