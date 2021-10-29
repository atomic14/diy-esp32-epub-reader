#ifdef USE_L58_TOUCH

#include "L58TouchControls.h"
#include "L58Touch.h"
#include <Renderer/Renderer.h>
#include "epd_driver.h"

void touchTask(void *param)
{
  L58TouchControls *controls = (L58TouchControls *)param;
  for (;;)
  {
    controls->ts->loop();
  }
}

// stash the instance - TODO - update the touch library to use std::function for callbacks so
// it can take lambdas
static L58TouchControls *instance = nullptr;

void touchHandler(TPoint p, TEvent e)
{
  if (e == TEvent::Tap && instance)
  {
    instance->handleTouch(p.x, p.y);
  }
}

L58TouchControls::L58TouchControls(Renderer *renderer, int touch_int, int width, int height, int rotation, ActionCallback_t on_action)
    : on_action(on_action), renderer(renderer)
{
  instance = this;
  this->ts = new L58Touch(touch_int);
  /** Instantiate touch. Important inject here the display width and height size in pixels
        setRotation(3)     Portrait mode */
  ts->begin(width, height);
  ts->setRotation(rotation);
  ts->setTapThreshold(50);
  ts->registerTouchHandler(touchHandler);
  xTaskCreate(touchTask, "touchTask", 4096, this, 0, NULL);
}

void L58TouchControls::render(Renderer *renderer)
{
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
  renderer->draw_circle(x_offset + (ui_button_width / 2) + 9, 15, 5, 0);
  renderer->set_margin_top(35);
}

void L58TouchControls::renderPressedState(Renderer *renderer, UIAction action, bool state)
{
  renderer->set_margin_top(0);
  switch (action)
  {
  case DOWN:
  {
    if (state)
    {
      renderer->fill_triangle(80, 20, 75, 6, 85, 6, 0);
    }
    else
    {
      renderer->fill_triangle(81, 19, 76, 7, 86, 7, 255);
    }
    renderer->flush_area(76, 6, 10, 15);
    break;
  }
  case UP:
  {
    if (state)
    {
      renderer->fill_triangle(220, 6, 220 - 5, 20, 220 + 5, 20, 0);
    }
    else
    {
      renderer->fill_triangle(221, 7, 221 - 5, 19, 221 + 5, 19, 255);
    }
    renderer->flush_area(195, 225, 10, 15);
  }
  break;
  case SELECT:
  {
    uint16_t x_circle = (ui_button_width * 2 + 60) + (ui_button_width / 2) + 9;
    renderer->fill_circle(x_circle, 15, 5, 0);
    renderer->flush_area(x_circle - 3, 12, 6, 6);
    // TODO - this causes a stack overflow when select is picked
    // renderPressedState(renderer, last_action, false);
  }
  break;
  case LAST_INTERACTION:
  case NONE:
    break;
  }
  renderer->set_margin_top(35);
}

void L58TouchControls::handleTouch(int x, int y)
{
  UIAction action = NONE;
  ESP_LOGI("TOUCH", "Received touch event %d,%d", x, y);
  if (x >= 10 && x <= 10 + ui_button_width && y < 200)
  {
    action = DOWN;
    renderPressedState(renderer, UP, false);
  }
  else if (x >= 150 && x <= 150 + ui_button_width && y < 200)
  {
    action = UP;
    renderPressedState(renderer, DOWN, false);
  }
  else if (x >= 300 && x <= 300 + ui_button_width && y < 200)
  {
    action = SELECT;
  }
  else
  {
    // Touched anywhere but not the buttons
    action = LAST_INTERACTION;
  }
  last_action = action;
  if (action != NONE)
  {
    on_action(action);
  }
}
#endif