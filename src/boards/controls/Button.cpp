#include "Button.h"
#include <esp_timer.h>
#include <driver/gpio.h>

// 100 ms debounce on the buttons
const int BUTTON_DEBOUNCE = 1000;

IRAM_ATTR void button_interrupt_handler(void *param)
{
  Button *button = (Button *)param;
  button->handle_interrupt();
}

Button::Button(gpio_num_t gpio_pin, int active_level, ButtonCallback_t callback)
    : gpio_pin(gpio_pin),
      active_level(active_level),
      callback(callback)
{
  // setup the pin
  gpio_set_direction(gpio_pin, GPIO_MODE_INPUT);
  gpio_set_pull_mode(gpio_pin, active_level == 0 ? GPIO_PULLUP_ONLY : GPIO_PULLDOWN_ONLY);
  // wire up the interrupt handler
  gpio_isr_handler_add(gpio_pin, button_interrupt_handler, this);
  // start off waiting for the button to be pressed - edge interrupts seem to be flaky so we
  // have to be a bit more clever
  button_pressed = false;
  gpio_set_intr_type(gpio_pin, active_level == 0 ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
  gpio_intr_enable(gpio_pin);
}

void Button::handle_interrupt()
{
  if (button_pressed)
  {
    // the button was pressed - so now it must have been released
    button_pressed = false;
    int64_t button_release_time = esp_timer_get_time();
    // has enough time passed to call this a button press?
    if (button_release_time - button_press_start > BUTTON_DEBOUNCE)
    {
      // call the callback
      callback();
    }
    // start listening for the button to be pressed again
    gpio_set_intr_type(gpio_pin, active_level == 0 ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
  }
  else
  {
    // the button was not pressed - so now it must be pressed
    button_pressed = true;
    button_press_start = esp_timer_get_time();
    // start listening for the button to be released
    gpio_set_intr_type(gpio_pin, active_level == 0 ? GPIO_INTR_HIGH_LEVEL : GPIO_INTR_LOW_LEVEL);
  }
}
