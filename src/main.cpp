#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "config.h"
#include "SDCard.h"
#include "Epub.h"

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

  while (true)
  {
    vTaskDelay(1000);
  }
}

void app_main()
{
  xTaskCreate(main_task, "main_task", 16384, NULL, 1, NULL);
}