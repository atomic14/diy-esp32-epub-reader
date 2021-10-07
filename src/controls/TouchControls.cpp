#include "L58Touch.cpp"
#include "TouchControls.h"
#include <Renderer/Renderer.h>

void touchTask(void *param)
{
  TouchControls *controls = (TouchControls *)param;
  for (;;)
  {
    controls->ts->loop();
    vTaskDelay(1);
  }
}

// this is a bit of a nasty hack
static TouchControls *instance = nullptr;

void touchHandler(TPoint p, TEvent e)
{
  if (e == TEvent::Tap && instance)
  {
    instance->eventX = p.x;
    instance->eventY = p.y;
    instance->tapFlag = true;
  }
}

TouchControls::TouchControls(int width, int height, int rotation)
{
#ifdef USE_TOUCH
  instance = this;
  this->ts = new L58Touch(CONFIG_TOUCH_INT);
  /** Instantiate touch. Important inject here the display width and height size in pixels
        setRotation(3)     Portrait mode */
  ts->begin(width, height);
  ts->setRotation(rotation);
  ts->registerTouchHandler(touchHandler);
  xTaskCreate(touchTask, "touchTask", 4096, this, 0, NULL);
#endif
}

void TouchControls::render(Renderer *renderer)
{
#ifdef USE_TOUCH
  renderer->set_margin_top(0);
  uint16_t x_offset = 10;
  uint16_t x_triangle = x_offset + 70;
  // DOWN
  renderer->draw_rect(x_offset, 1, ui_button_width, ui_button_height, 0);
  renderer->draw_triangle(x_triangle, 20, x_triangle - 5, 6, x_triangle + 5, 6, 0);
  // UP
  x_offset = ui_button_width + 30;
  x_triangle = x_offset + 70;
  renderer->draw_rect(x_offset, 1, ui_button_width, ui_button_height, 0);
  renderer->draw_triangle(x_triangle, 6, x_triangle - 5, 20, x_triangle + 5, 20, 0);
  // SELECT
  x_offset = ui_button_width * 2 + 60;
  renderer->draw_rect(x_offset, 1, ui_button_width, ui_button_height, 0);
  renderer->fill_circle(x_offset + (ui_button_width / 2) + 9, 15, 5, 0);
  renderer->set_margin_top(35);
#endif
}

UIAction TouchControls::get_action()
{
  UIAction action = NONE;
#ifdef USE_TOUCH
  // Touch event detected: Override ui_action
  if (tapFlag && !(lastX == eventX && lastY == eventY))
  {
    // Note: Not always works overriding the ui_action here although Tap is detected fine
    if (eventX >= 10 && eventX <= 10 + ui_button_width && eventY < 60)
    {
      action = DOWN;
    }
    else if (eventX >= 150 && eventX <= 150 + ui_button_width && eventY < 60)
    {
      action = UP;
    }
    else if (eventX >= 300 && eventX <= 300 + ui_button_width && eventY < 60)
    {
      action = SELECT;
    }
  }
  // Avoid repeated touchs FIX this to make it better
  lastX = eventX;
  lastY = eventY;
  tapFlag = false;
#endif
  return action;
}
