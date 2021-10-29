#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include <driver/gpio.h>
#include <esp32/ulp.h>
#include <esp_log.h>
#include <esp_timer.h>
#include "ButtonControlsEpdiyV6.h"
#include "ulp_main.h"

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

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");

ButtonControls::ButtonControls(
    gpio_num_t gpio_down,
    gpio_num_t gpio_select,
    int active_level,
    ActionCallback_t on_action)
    : gpio_down(gpio_down), gpio_select(gpio_select),
      active_level(active_level),
      on_action(on_action)
{
  gpio_install_isr_service(0);

  down = new Button(gpio_down, active_level, [this]()
                    { this->on_action(UIAction::DOWN); });
  select = new Button(gpio_select, active_level, [this]()
                      { this->on_action(UIAction::SELECT); });
}

bool ButtonControls::did_wake_from_deep_sleep()
{
  auto wake_cause = esp_sleep_get_wakeup_cause();
  // if our controls are active low then we must have been woken by the ULP
  // as that's the only mechanism available for active low buttons
  if (active_level == 0 && wake_cause == ESP_SLEEP_WAKEUP_ULP)
  {
    ESP_LOGI("Controls", "ULP Wakeup");
    return true;
  }
  if (active_level == 1 && wake_cause == ESP_SLEEP_WAKEUP_EXT1)
  {
    ESP_LOGI("Controls", "EXT1 Wakeup");
    return true;
  }
  return false;
}

UIAction ButtonControls::get_deep_sleep_action()
{
  if (active_level == 0)
  {
    uint16_t rtc_pin = ulp_gpio_status & UINT16_MAX;
    ESP_LOGI("Controls", "***** rtc_pin: %d", rtc_pin);
    if ((rtc_pin & (1 << rtc_io_number_get(gpio_down))))
    {
      ESP_LOGI("Controls", "***** DOWN %d, %d, %d", 1 << rtc_io_number_get(gpio_down), rtc_pin, (rtc_pin & (1 << rtc_io_number_get(gpio_down))));
      return UIAction::DOWN;
    }
    else if ((rtc_pin & (1 << rtc_io_number_get(gpio_select))))
    {
      ESP_LOGI("Controls", "***** SELECT %d, %d, %d", 1 << rtc_io_number_get(gpio_select), rtc_pin, (rtc_pin & (1 << rtc_io_number_get(gpio_select))));
      return UIAction::SELECT;
    }
  }
  else
  {
    uint64_t ext1_buttons = esp_sleep_get_ext1_wakeup_status();
    if (ext1_buttons & (1ULL << gpio_down))
    {
      return UIAction::DOWN;
    }
    else if (ext1_buttons & (1ULL << gpio_select))
    {
      return UIAction::SELECT;
    }
  }
  return UIAction::NONE;
}

void ButtonControls::setup_deep_sleep()
{
  if (active_level == 0)
  {
    rtc_gpio_init(gpio_down);
    rtc_gpio_set_direction(gpio_down, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_en(gpio_down);

    rtc_gpio_init(gpio_select);
    rtc_gpio_set_direction(gpio_select, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_en(gpio_select);

    // need to use the ULP if we have buttons that are active low
    // see ulp/main.S for more details
    esp_err_t err = ulp_load_binary(0, ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);

    ulp_down_mask = 1 << rtc_io_number_get(gpio_down);
    ulp_select_mask = 1 << rtc_io_number_get(gpio_select);

    ulp_set_wakeup_period(0, 100 * 1000); // 100 ms
    err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
    ESP_ERROR_CHECK(err);
  }
  else
  {
    // can use ext1 for buttons that are active high

    rtc_gpio_init(gpio_down);
    rtc_gpio_set_direction(gpio_down, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pulldown_en(gpio_down);

    rtc_gpio_init(gpio_select);
    rtc_gpio_set_direction(gpio_select, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pulldown_en(gpio_select);
    esp_sleep_enable_ext1_wakeup(
        (1ULL << gpio_down) | (1ULL << gpio_select),
        ESP_EXT1_WAKEUP_ANY_HIGH);
  }
}

esp_err_t ButtonControls::i2c_master_read_slave(i2c_port_t i2c_num, uint8_t* data_rd, size_t size, int reg)
{
    if (size == 0) {
        return ESP_OK;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ( EPDIY_PCA9555_ADDR << 1 ) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, reg, true);
	i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    if (ret != ESP_OK) {
        return ret;
    }
	i2c_cmd_link_delete(cmd);

	cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
	i2c_master_write_byte(cmd, ( EPDIY_PCA9555_ADDR << 1 ) | I2C_MASTER_READ, true);
	if (size > 1) {
        i2c_master_read(cmd, data_rd, size - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data_rd + size - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

	ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    if (ret != ESP_OK) {
        return ret;
    }
    i2c_cmd_link_delete(cmd);

    return ESP_OK;
}

uint8_t ButtonControls::pca9555_read_input(i2c_port_t i2c_port, int high_port) {
    esp_err_t err;
	uint8_t r_data[1];

    err = i2c_master_read_slave(i2c_port, r_data, 1, REG_INPUT_PORT0 + high_port);
    if (err != ESP_OK) {
        ESP_LOGE("PCA9555", "%s failed", __func__);
        return 0;
    }

	return r_data[0];
}
