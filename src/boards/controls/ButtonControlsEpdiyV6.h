#pragma once

#include "Actions.h"
#include <hal/gpio_types.h>

typedef std::function<void(void)> ButtonCallback_t;

class Button
{
private:
  gpio_num_t gpio_pin;
  int active_level;

  friend void button_interrupt_handler(void *param);

  int64_t button_press_start = 0;
  bool button_pressed = false;

  ButtonCallback_t callback;

  void handle_interrupt();

public:
  Button(gpio_num_t gpio_pin, int active_level, ButtonCallback_t callback);
};

class ButtonControls
{
private:
  gpio_num_t gpio_down;
  gpio_num_t gpio_select;
  int active_level;

  Button *down;
  Button *select;

  ActionCallback_t on_action;

public:
  ButtonControls(
      gpio_num_t gpio_down,
      gpio_num_t gpio_select,
      int active_level,
      ActionCallback_t on_action);
  bool did_wake_from_deep_sleep();
  UIAction get_deep_sleep_action();
  void setup_deep_sleep();
};