#include <esp_log.h>
#include "Board.h"
#include "Epdiy.h"
#include "Lilygo_t5_47.h"
#include "M5Paper.h"
#include <SPIFFS.h>
#include <SDCard.h>
#include "battery/ADCBattery.h"
#include "controls/GPIOButtonControls.h"

Board *Board::factory()
{
#ifdef BOARD_TYPE_LILIGO_T5_47
  return new Lilygo_t5_47();
#endif
#ifdef BOARD_TYPE_EPDIY
  return new Epdiy();
#endif
#ifdef BOARD_TYPE_M5_PAPER
  return new M5Paper();
#endif
}

void Board::start_filesystem()
{
  // create the EPD renderer
#ifdef USE_SPIFFS
  ESP_LOGI("main", "Using SPIFFS");
  // create the file system
  spiffs = new SPIFFS("/fs");
#else
  ESP_LOGI("main", "Using SDCard");
  // initialise the SDCard
  sdcard = new SDCard("/fs", SD_CARD_PIN_NUM_MISO, SD_CARD_PIN_NUM_MOSI, SD_CARD_PIN_NUM_CLK, SD_CARD_PIN_NUM_CS);
#endif
}

void Board::stop_filesystem()
{
#ifdef USE_SPIFFS
  delete spiffs;
#else
  delete sdcard;
#endif
}

Battery *Board::get_battery()
{
#ifdef BATTERY_ADC_CHANNEL
  return new ADCBattery(BATTERY_ADC_CHANNEL);
#else
  return nullptr;
#endif
}

TouchControls *Board::get_touch_controls(Renderer *renderer, xQueueHandle ui_queue)
{
  return new TouchControls();
}
