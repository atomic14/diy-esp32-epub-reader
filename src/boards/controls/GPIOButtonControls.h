#pragma once

#include "Actions.h"
#include <hal/gpio_types.h>
#include "GPIOButton.h"
#include "ButtonControls.h"

class GPIOButtonControls : public ButtonControls
{
private:
  gpio_num_t gpio_up;
  gpio_num_t gpio_down;
  gpio_num_t gpio_select;
  int active_level;

  GPIOButton *up;
  GPIOButton *down;
  GPIOButton *select;

  ActionCallback_t on_action;

public:
  GPIOButtonControls(
      gpio_num_t gpio_up,
      gpio_num_t gpio_down,
      gpio_num_t gpio_select,
      int active_level,
      ActionCallback_t on_action);
  bool did_wake_from_deep_sleep();
  UIAction get_deep_sleep_action();
  void setup_deep_sleep();
};