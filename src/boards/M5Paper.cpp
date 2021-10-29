#include "M5Paper.h"
#include <Renderer/M5PaperRenderer.h>
#include <regular_font.h>
#include <bold_font.h>
#include <italic_font.h>
#include <bold_italic_font.h>
#include <hourglass.h>
#include <SPIFFS.h>

#define M5EPD_MAIN_PWR_PIN GPIO_NUM_2

void M5Paper::power_up()
{
  // nothing to do for this board - should probably move the power up from
  // the driver to here?
}
void M5Paper::prepare_to_sleep()
{
  // keep the power on as we are going into deep sleep not shutting down completely
  rtc_gpio_init(M5EPD_MAIN_PWR_PIN);
  rtc_gpio_set_direction(M5EPD_MAIN_PWR_PIN, RTC_GPIO_MODE_OUTPUT_ONLY);
  rtc_gpio_set_level(M5EPD_MAIN_PWR_PIN, 1);
  rtc_gpio_hold_en(M5EPD_MAIN_PWR_PIN);
}
Renderer *M5Paper::get_renderer()
{
  return new M5PaperRenderer(
      &regular_font,
      &bold_font,
      &italic_font,
      &bold_italic_font,
      hourglass_data,
      hourglass_width,
      hourglass_height);
}
void M5Paper::stop_filesystem()
{
#ifdef USE_SPIFFS
  delete spiffs;
#else
  // seems to cause issues with the M5 Paper
  // delete sdcard;
#endif
}