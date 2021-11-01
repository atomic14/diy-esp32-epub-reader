#include "Epdiy.h"
#include <Renderer/EpdiyRenderer.h>
#include <regular_font.h>
#include <bold_font.h>
#include <italic_font.h>
#include <bold_italic_font.h>
#include <hourglass.h>
#include "controls/EpdiyV6ButtonControls.h"

// setup the pins to use for navigation
// Only select is managed in EPDiy V6 version, up and down are read by I2C
#define BUTTON_SELECT_GPIO_NUM GPIO_NUM_39
//buttons are low when pressed (For some reason in EPDiy on low it wakes up directly from deepsleep)
#define BUTONS_ACTIVE_LEVEL 1

void Epdiy::power_up()
{
  // nothing to do for this board - do not call epd_poweron in this case
}
void Epdiy::prepare_to_sleep()
{
  epd_poweroff();
}
Renderer *Epdiy::get_renderer()
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

ButtonControls *Epdiy::get_button_controls(xQueueHandle ui_queue)
{
  return new EpdiyV6ButtonControls(
      BUTTON_SELECT_GPIO_NUM,
      BUTONS_ACTIVE_LEVEL,
      [ui_queue](UIAction action)
      {
        xQueueSend(ui_queue, &action, 0);
      });
}