#include "L58Touch.cpp"
#include "TouchControls.h"
#include <Renderer/Renderer.h>

typedef struct
{
  int eventX;
  int eventY;
} TouchEvent;

void touchTask(void *param)
{
  TouchControls *controls = (TouchControls *)param;
  for (;;)
  {
    controls->ts->loop();
    vTaskDelay(1);
  }
}

QueueHandle_t touchQueue = nullptr;

void touchHandler(TPoint p, TEvent e)
{
  if (e == TEvent::Tap && touchQueue)
  {
    // for now we'll just alow one touch event - we could use this to queue up taps at some point
    if (uxQueueMessagesWaiting(touchQueue) == 0)
    {
      TouchEvent event = {
          .eventX = p.x,
          .eventY = p.y,
      };
      xQueueSend(touchQueue, &event, 0);
    }
  }
}

TouchControls::TouchControls(int width, int height, int rotation)
{
#ifdef USE_TOUCH
  this->ts = new L58Touch(CONFIG_TOUCH_INT);
  /** Instantiate touch. Important inject here the display width and height size in pixels
        setRotation(3)     Portrait mode */
  ts->begin(width, height);
  ts->setRotation(rotation);
  ts->registerTouchHandler(touchHandler);
  touchQueue = xQueueCreate(1, sizeof(TouchEvent));
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
  TouchEvent event;
  if (xQueueReceive(touchQueue, &event, 0))
  {
    if (event.eventX >= 10 && event.eventX <= 10 + ui_button_width && event.eventY < 60)
    {
      action = DOWN;
    }
    else if (event.eventX >= 150 && event.eventX <= 150 + ui_button_width && event.eventY < 60)
    {
      action = UP;
    }
    else if (event.eventX >= 300 && event.eventX <= 300 + ui_button_width && event.eventY < 60)
    {
      action = SELECT;
    }
  }
#endif
  return action;
}
