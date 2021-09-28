#pragma once

#include <driver/gpio.h>

class Battery
{
public:
  Battery(gpio_pin_t pin);
  float get_voltage();
  float get_percentage();
}