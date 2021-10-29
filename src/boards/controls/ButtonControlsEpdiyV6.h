#pragma once
#include "pca9555.h"
#include <driver/i2c.h>
#include "Actions.h"
#include <hal/gpio_types.h>

#define REG_INPUT_PORT0      0
#define BUTTON_IO_DOWN  (PCA_PIN_P07 >> 8)

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
  // I2C related for IO expander PCA9555
  static const int EPDIY_PCA9555_ADDR = 0x20;
  
  esp_err_t i2c_master_read_slave(i2c_port_t i2c_num, uint8_t* data_rd, size_t size, int reg);
  uint8_t pca9555_read_input(i2c_port_t port, int high_port);
  // Note: What might be missing here is to set the PCA button ports as input
  //       If needed then also I2C write functions should be added  
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