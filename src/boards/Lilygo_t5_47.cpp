#include "Lilygo_t5_47.h"
#include <regular_font.h>
#include <bold_font.h>
#include <italic_font.h>
#include <bold_italic_font.h>
#include <hourglass.h>
#ifdef USE_L58_TOUCH
#include "controls/L58TouchControls.h"
#endif
#include <Renderer/EpdiyRenderer.h>
#include "controls/GPIOButtonControls.h"

// setup the pins to use for navigation
#define BUTTON_UP_GPIO_NUM GPIO_NUM_34
#define BUTTON_DOWN_GPIO_NUM GPIO_NUM_39
#define BUTTON_SELECT_GPIO_NUM GPIO_NUM_35
// buttons are low when pressed
#define BUTONS_ACTIVE_LEVEL 0

void Lilygo_t5_47::power_up()
{
  // Need to power on the EDP to get power to the SD Card
  epd_poweron();
}
void Lilygo_t5_47::prepare_to_sleep()
{
  epd_poweroff();
}
Renderer *Lilygo_t5_47::get_renderer()
{
  return new EpdiyRenderer(
      &regular_font,
      &bold_font,
      &italic_font,
      &bold_italic_font,
      hourglass_data,
      hourglass_width,
      hourglass_height);
}
TouchControls *Lilygo_t5_47::get_touch_controls(Renderer *renderer, xQueueHandle ui_queue)
{
#ifdef USE_L58_TOUCH
  return new L58TouchControls(
      renderer,
      CONFIG_TOUCH_INT,
      EPD_WIDTH,
      EPD_HEIGHT,
      3,
      [ui_queue](UIAction action)
      {
        xQueueSend(ui_queue, &action, 0);
      });
#endif
  // dummy implementation
  return new TouchControls();
}
ButtonControls *Lilygo_t5_47::get_button_controls(xQueueHandle ui_queue)
{
  return new GPIOButtonControls(
      BUTTON_UP_GPIO_NUM,
      BUTTON_DOWN_GPIO_NUM,
      BUTTON_SELECT_GPIO_NUM,
      BUTONS_ACTIVE_LEVEL,
      [ui_queue](UIAction action)
      {
        xQueueSend(ui_queue, &action, 0);
      });
}