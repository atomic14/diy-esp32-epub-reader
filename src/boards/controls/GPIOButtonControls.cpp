// Exclude this class if the board is EPDIY
#ifndef BOARD_TYPE_EPDIY
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include <driver/gpio.h>
#include <esp32/ulp.h>
#include <esp_log.h>
#include <esp_timer.h>
#include "GPIOButtonControls.h"
#include "ulp_main.h"

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");

GPIOButtonControls::GPIOButtonControls(
    gpio_num_t gpio_up,
    gpio_num_t gpio_down,
    gpio_num_t gpio_select,
    int active_level,
    ActionCallback_t on_action)
    : gpio_up(gpio_up), gpio_down(gpio_down), gpio_select(gpio_select),
      active_level(active_level),
      on_action(on_action)
{
  gpio_install_isr_service(0);
  up = new GPIOButton(gpio_up, active_level, [this]()
                      { this->on_action(UIAction::UP); });
  down = new GPIOButton(gpio_down, active_level, [this]()
                        { this->on_action(UIAction::DOWN); });
  select = new GPIOButton(gpio_select, active_level, [this]()
                          { this->on_action(UIAction::SELECT); });
}

bool GPIOButtonControls::did_wake_from_deep_sleep()
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

UIAction GPIOButtonControls::get_deep_sleep_action()
{
  if (active_level == 0)
  {
    uint16_t rtc_pin = ulp_gpio_status & UINT16_MAX;
    ESP_LOGI("Controls", "***** rtc_pin: %d", rtc_pin);
    if ((rtc_pin & (1 << rtc_io_number_get(gpio_up))))
    {
      ESP_LOGI("Controls", "***** UP %d, %d, %d", 1 << rtc_io_number_get(gpio_up), rtc_pin, (rtc_pin & (1 << rtc_io_number_get(gpio_up))));
      return UIAction::UP;
    }
    else if ((rtc_pin & (1 << rtc_io_number_get(gpio_down))))
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
    if (ext1_buttons & (1ULL << gpio_up))
    {
      return UIAction::UP;
    }
    else if (ext1_buttons & (1ULL << gpio_down))
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

void GPIOButtonControls::setup_deep_sleep()
{
  if (active_level == 0)
  {
    rtc_gpio_init(gpio_up);
    rtc_gpio_set_direction(gpio_up, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_en(gpio_up);

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

    ulp_up_mask = 1 << rtc_io_number_get(gpio_up);
    ulp_down_mask = 1 << rtc_io_number_get(gpio_down);
    ulp_select_mask = 1 << rtc_io_number_get(gpio_select);

    ulp_set_wakeup_period(0, 100 * 1000); // 100 ms
    err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
    ESP_ERROR_CHECK(err);
  }
  else
  {
    // can use ext1 for buttons that are active high
    rtc_gpio_init(gpio_up);
    rtc_gpio_set_direction(gpio_up, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pulldown_en(gpio_up);

    rtc_gpio_init(gpio_down);
    rtc_gpio_set_direction(gpio_down, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pulldown_en(gpio_down);

    rtc_gpio_init(gpio_select);
    rtc_gpio_set_direction(gpio_select, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pulldown_en(gpio_select);
    esp_sleep_enable_ext1_wakeup(
        (1ULL << gpio_up) | (1ULL << gpio_down) | (1ULL << gpio_select),
        ESP_EXT1_WAKEUP_ANY_HIGH);
  }
}
#endif