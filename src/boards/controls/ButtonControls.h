#pragma once

#include "Actions.h"
#include <hal/gpio_types.h>
#include "Button.h"

class ButtonControls
{
private:
  gpio_num_t gpio_up;
  gpio_num_t gpio_down;
  gpio_num_t gpio_select;
  int active_level;

  Button *up;
  Button *down;
  Button *select;

  ActionCallback_t on_action;

public:
  ButtonControls(
      gpio_num_t gpio_up,
      gpio_num_t gpio_down,
      gpio_num_t gpio_select,
      int active_level,
      ActionCallback_t on_action);
  bool did_wake_from_deep_sleep();
  UIAction get_deep_sleep_action();
  void setup_deep_sleep();
};