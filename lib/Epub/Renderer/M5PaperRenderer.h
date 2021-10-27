#pragma once
#include <esp_log.h>
#include <M5EPD_Driver.h>
#include <driver/rtc_io.h>
#include <driver/gpio.h>
#include "EpdiyFrameBufferRenderer.h"
#include "miniz.h"

#define M5EPD_CS_PIN GPIO_NUM_15
#define M5EPD_SCK_PIN GPIO_NUM_14
#define M5EPD_MOSI_PIN GPIO_NUM_12
#define M5EPD_BUSY_PIN GPIO_NUM_27
#define M5EPD_MISO_PIN GPIO_NUM_13
#define M5EPD_MAIN_PWR_PIN GPIO_NUM_2
#define M5EPD_EXT_PWR_EN_PIN GPIO_NUM_5
#define M5EPD_EPD_PWR_EN_PIN GPIO_NUM_23

class M5PaperRenderer : public EpdiyFrameBufferRenderer
{
private:
  M5EPD_Driver driver;

public:
  M5PaperRenderer(
      const EpdFont *regular_font,
      const EpdFont *bold_font,
      const EpdFont *italic_font,
      const EpdFont *bold_italic_font,
      const uint8_t *busy_icon,
      int busy_icon_width,
      int busy_icon_height)
      : EpdiyFrameBufferRenderer(regular_font, bold_font, italic_font, bold_italic_font, busy_icon, busy_icon_width, busy_icon_height)
  {
    // gpio_set_direction(M5EPD_EXT_PWR_EN_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(M5EPD_EPD_PWR_EN_PIN, GPIO_MODE_OUTPUT);
    // gpio_set_level(M5EPD_EXT_PWR_EN_PIN, 1);
    gpio_set_level(M5EPD_EPD_PWR_EN_PIN, 1);

    driver.begin(M5EPD_SCK_PIN, M5EPD_MOSI_PIN, M5EPD_MISO_PIN, M5EPD_CS_PIN, M5EPD_BUSY_PIN);
    driver.SetColorReverse(true);
    driver.Clear(true);
  }
  ~M5PaperRenderer()
  {
    // TODO: cleanup and shutdown?
  }
  void flush_display()
  {
    driver.WriteFullGram4bpp(m_frame_buffer);
    driver.UpdateFull(needs_gray_flush ? UPDATE_MODE_GC16 : UPDATE_MODE_DU);
    needs_gray_flush = false;
  }
  void flush_area(int x, int y, int width, int height)
  {
    // TODO - work out how to do a partial update
    flush_display();
  }
  virtual void reset()
  {
    ESP_LOGI("EPD", "Full clear");
    clear_screen();
    // flushing to white
    needs_gray_flush = false;
    flush_display();
  };
};