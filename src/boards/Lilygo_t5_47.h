#pragma once

#include "Board.h"

class Lilygo_t5_47 : public Board
{
public:
  virtual void power_up();
  virtual void prepare_to_sleep();
  virtual Renderer *get_renderer();
  virtual TouchControls *get_touch_controls(Renderer *renderer, xQueueHandle ui_queue);
  virtual ButtonControls *get_button_controls(xQueueHandle ui_queue);
};