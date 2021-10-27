#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include <driver/gpio.h>
#include <esp32/ulp.h>
#include <esp_log.h>
#include <esp_timer.h>
#include "ButtonControls.h"
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
  up = new Button(gpio_up, active_level, [this]()
                  { this->on_action(UIAction::UP); });
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

void ButtonControls::setup_deep_sleep()
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

    rtc_gpio_init(GPIO_NUM_2);
    rtc_gpio_set_direction(GPIO_NUM_2, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level(GPIO_NUM_2, 1);
    rtc_gpio_hold_en(GPIO_NUM_2);
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