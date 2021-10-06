#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include <driver/gpio.h>
#include <esp32/ulp.h>
#include <esp_log.h>
#include "Controls.h"
#include "ulp_main.h"

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");

Controls::Controls(gpio_num_t up, gpio_num_t down, gpio_num_t select, int active_level) : up(up), down(down), select(select), active_level(active_level)
{
}

void Controls::setup_inputs()
{
  gpio_set_direction(up, GPIO_MODE_INPUT);
  gpio_set_direction(down, GPIO_MODE_INPUT);
  gpio_set_direction(select, GPIO_MODE_INPUT);

  if (active_level == 0)
  {
    gpio_set_pull_mode(up, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(down, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(select, GPIO_PULLUP_ONLY);
  }
  else
  {
    gpio_set_pull_mode(up, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(down, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(select, GPIO_PULLDOWN_ONLY);
  }
}

UIAction Controls::get_action()
{
  if (gpio_get_level(up) == active_level)
  {
    return UIAction::UP;
  }
  if (gpio_get_level(down) == active_level)
  {
    return UIAction::DOWN;
  }
  if (gpio_get_level(select) == active_level)
  {
    return UIAction::SELECT;
  }
  return UIAction::NONE;
}

bool Controls::did_wake_from_deep_sleep()
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

UIAction Controls::get_deep_sleep_action()
{
  if (active_level == 0)
  {
    uint16_t rtc_pin = ulp_gpio_status & UINT16_MAX;
    ESP_LOGI("Controls", "***** rtc_pin: %d", rtc_pin);
    if ((rtc_pin & (1 << rtc_io_number_get(up))))
    {
      ESP_LOGI("Controls", "***** UP %d, %d, %d", 1 << rtc_io_number_get(up), rtc_pin, (rtc_pin & (1 << rtc_io_number_get(up))));
      return UIAction::UP;
    }
    else if ((rtc_pin & (1 << rtc_io_number_get(down))))
    {
      ESP_LOGI("Controls", "***** DOWN %d, %d, %d", 1 << rtc_io_number_get(down), rtc_pin, (rtc_pin & (1 << rtc_io_number_get(down))));
      return UIAction::DOWN;
    }
    else if ((rtc_pin & (1 << rtc_io_number_get(select))))
    {
      ESP_LOGI("Controls", "***** SELECT %d, %d, %d", 1 << rtc_io_number_get(select), rtc_pin, (rtc_pin & (1 << rtc_io_number_get(select))));
      return UIAction::SELECT;
    }
  }
  else
  {
    uint64_t ext1_buttons = esp_sleep_get_ext1_wakeup_status();
    if (ext1_buttons & (1ULL << up))
    {
      return UIAction::UP;
    }
    else if (ext1_buttons & (1ULL << down))
    {
      return UIAction::DOWN;
    }
    else if (ext1_buttons & (1ULL << select))
    {
      return UIAction::SELECT;
    }
  }
  return UIAction::NONE;
}

void Controls::setup_deep_sleep()
{
  if (active_level == 0)
  {
    rtc_gpio_init(up);
    rtc_gpio_set_direction(up, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pulldown_dis(up);
    rtc_gpio_pullup_dis(up);
    rtc_gpio_hold_dis(up);

    rtc_gpio_init(down);
    rtc_gpio_set_direction(down, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pulldown_dis(down);
    rtc_gpio_pullup_dis(down);
    rtc_gpio_hold_dis(down);

    rtc_gpio_init(select);
    rtc_gpio_set_direction(select, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pulldown_dis(select);
    rtc_gpio_pullup_dis(select);
    rtc_gpio_hold_dis(select);
    // need to use the ULP if we have buttons that are active low
    // see ulp/main.S for more details

    esp_err_t err = ulp_load_binary(0, ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);

    ulp_up_mask = 1 << rtc_io_number_get(up);
    ulp_down_mask = 1 << rtc_io_number_get(down);
    ulp_select_mask = 1 << rtc_io_number_get(select);

    ulp_set_wakeup_period(0, 100 * 1000); // 100 ms
    err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
    ESP_ERROR_CHECK(err);
  }
  else
  {
    // can use ext1 for buttons that are active high
    esp_sleep_enable_ext1_wakeup(
        (1ULL << up) | (1ULL << down) | (1ULL << select),
        ESP_EXT1_WAKEUP_ANY_HIGH);
  }
}