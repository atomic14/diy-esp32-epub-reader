#include "Battery.h"

Battery::Battery(gpio_pin_t pin)
{
  adc1_config_width(ADC_WIDTH_12Bit);
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
}

float get_voltage()
{
  // get the adc calibration
  auto adc_value = adc1_get_raw(ADC1_CHANNEL_0);
  // get the actual calibrated voltage
  auto voltage = 2 * esp_adc_cal_raw_to_voltage(adc_value, &adc_chars);
  return voltage;
}

// see here for inspiration: https://github.com/G6EJD/ESP32-e-Paper-Weather-Display/issues/146
float get_percentage()
{
  auto voltage = get_voltage();
  if (voltage >= 4.20)
  {
    return 100;
  }
  if (voltage <= 3.50)
  {
    return 0;
  }
  return 2836.9625 * pow(voltage, 4) - 43987.4889 * pow(voltage, 3) + 255233.8134 * pow(voltage, 2) - 656689.7123 * voltage + 632041.7303;
}