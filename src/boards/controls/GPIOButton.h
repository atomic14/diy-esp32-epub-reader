#pragma once

#include <hal/gpio_types.h>
#include <functional>

typedef std::function<void(void)> ButtonCallback_t;

class GPIOButton
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
  GPIOButton(gpio_num_t gpio_pin, int active_level, ButtonCallback_t callback);
};
