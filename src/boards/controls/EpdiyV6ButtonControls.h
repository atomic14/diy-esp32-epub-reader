#pragma once
#include "pca9555.h"
#include <driver/i2c.h>
#include "Actions.h"
#include "GPIOButton.h"
#include <esp_log.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include <driver/gpio.h>
#include <esp32/ulp.h>
#include <esp_timer.h>
#include "ulp_main.h"
#include "ButtonControls.h"

// This CFG_INTR is defined on display_ops.h
#define CFG_INTR GPIO_NUM_35
#define EPDIY_I2C_PORT I2C_NUM_0
#define REG_INPUT_PORT0 0
#define BUTTON_IO_PORT (PCA_PIN_P00 >> 8)

class EpdiyV6ButtonControls : public ButtonControls
{
private:
  gpio_num_t gpio_select;
  int active_level;

  GPIOButton *select;

  ActionCallback_t on_action;
  // I2C related for IO expander PCA9555
  static const int EPDIY_PCA9555_ADDR = 0x20;

  esp_err_t i2c_master_read_slave(i2c_port_t i2c_num, uint8_t *data_rd, size_t size, int reg);
  uint8_t pca9555_read_input(i2c_port_t port, int high_port);

  // Note: pca9555 IO ports are declared as input as default. Important: IOs should be pulled-up to 3.3v
  // IO returns: 1 when IO0 is released. 2 when IO1 is released (PCA input ports)
  static void control_task(void *param)
  {
    EpdiyV6ButtonControls *bc = (EpdiyV6ButtonControls *)param;
    for (;;)
    {
      if (gpio_get_level(CFG_INTR) == 0)
      {
        uint8_t read = bc->pca9555_read_input(EPDIY_I2C_PORT, BUTTON_IO_PORT);

        if (read == 2)
        {
          bc->on_action(UIAction::UP);
        }
        else if (read == 1)
        {
          bc->on_action(UIAction::DOWN);
        }

        ESP_LOGI("Controls", "Read I2C:%d\n", read);
      }
      vTaskDelay(pdMS_TO_TICKS(50));
    }
  }

public:
  EpdiyV6ButtonControls(
      gpio_num_t gpio_select,
      int active_level,
      ActionCallback_t on_action);
  bool did_wake_from_deep_sleep();
  UIAction get_deep_sleep_action();
  void setup_deep_sleep();
};