#pragma once

#include "Actions.h"

class Controls
{
private:
  gpio_num_t up;
  gpio_num_t down;
  gpio_num_t select;

  int active_level;

public:
  Controls(gpio_num_t up, gpio_num_t down, gpio_num_t select, int active_level);
  void setup_inputs();
  bool did_wake_from_deep_sleep();
  UIAction get_action();
  UIAction get_deep_sleep_action();
  void setup_deep_sleep();
};