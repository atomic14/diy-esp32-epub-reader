#pragma once

#include <driver/adc.h>
#include <esp_adc_cal.h>

class Battery
{
private:
  adc1_channel_t m_adc_channel;
  esp_adc_cal_characteristics_t m_adc_chars;

public:
  Battery(adc1_channel_t adc_channel);
  float get_voltage();
  float get_percentage();
};