#pragma once
#include "pca9555.h"
#include <driver/i2c.h>
#include "Actions.h"
#include "Button.h"
#include <esp_log.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include <driver/gpio.h>
#include <esp32/ulp.h>
#include <esp_timer.h>
#include "ulp_main.h"

// This CFG_INTR is defined on display_ops.h
#define CFG_INTR GPIO_NUM_35
#define EPDIY_I2C_PORT I2C_NUM_0
#define REG_INPUT_PORT0      0
#define BUTTON_IO_DOWN  (PCA_PIN_P07 >> 8)

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

  // Note: pca9555 IO ports are declared as input as default. Important: IOs should be pulled-up to 3.3v 
  // IO returns: 0 in button pressed connecting GND and 128 in released state
  static void control_task(void *param) {
    ButtonControls *bc = (ButtonControls*) param;
    for (;;) {
      if (gpio_get_level(CFG_INTR) == 0) {
        uint8_t read = bc->pca9555_read_input(EPDIY_I2C_PORT, BUTTON_IO_DOWN);
        ESP_LOGI("Controls", "UP value:%d\n", read);
        if (read == 0) {
          bc->on_action(UIAction::UP);
        }
      }
      vTaskDelay(pdMS_TO_TICKS(50));
    }
  }

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