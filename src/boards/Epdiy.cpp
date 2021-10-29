#include "Epdiy.h"
#include <Renderer/EpdiyRenderer.h>
#include <regular_font.h>
#include <bold_font.h>
#include <italic_font.h>
#include <bold_italic_font.h>
#include <hourglass.h>

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