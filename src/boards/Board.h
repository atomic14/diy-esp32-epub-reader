#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <stdint.h>
#include "controls/ButtonControls.h"
#include "controls/TouchControls.h"
#include "battery/Battery.h"

class SPIFFS;
class SDCard;
class Renderer;

class Board
{
protected:
#ifdef USE_SPIFFS
  SPIFFS *spiffs;
#else
  SDCard *sdcard;
#endif

public:
  // override this to do any startup tasks required for your board
  // e.g. turning on the epd, enabling power to peripherals, etc...
  virtual void power_up() = 0;
  // override this to do any shutdown tasks required for your board
  // e.g. turning off the epd, disabling power to peripherals, etc...
  virtual void prepare_to_sleep() = 0;
  // get the renderer for your board
  virtual Renderer *get_renderer() = 0;
  // start up the filesystem - default behaviour is to use SPIFFS if USE_SPIFFS is defined
  // otherwise use the SD card with the pins defined in platformio.ini
  virtual void start_filesystem();
  // stop the filesystem
  virtual void stop_filesystem();
  // get the battery monitoring object - the default behaviour is to use the build in
  // ADC if BATTERY_ADC_CHANNEL is defined
  virtual Battery *get_battery();

  // get the button controls object
  virtual ButtonControls *get_button_controls(xQueueHandle ui_queue) = 0;

  // implement this to return a TouchControls object for your board - the default behaviour returns
  // a dummy implementation that does nothing
  virtual TouchControls *get_touch_controls(Renderer *renderer, xQueueHandle ui_queue);

  // factory method to create a new instance of the board - uses the BOARD_TYPE_* define from platformio.ini
  static Board *factory();
};