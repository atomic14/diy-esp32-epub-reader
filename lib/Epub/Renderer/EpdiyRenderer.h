#pragma once
#include <esp_log.h>
#include <epd_driver.h>
#include <epd_highlevel.h>
#include <math.h>
#include "EpdFrameBufferRenderer.h"
#include "miniz.h"

class EpdiyRenderer : public EpdFrameBufferRenderer
{
public:
  EpdiyRenderer(
      const EpdFont *regular_font,
      const EpdFont *bold_font,
      const EpdFont *italic_font,
      const EpdFont *bold_italic_font,
      const uint8_t *busy_icon,
      int busy_icon_width,
      int busy_icon_height)
      : EpdFrameBufferRenderer(regular_font, bold_font, italic_font, bold_italic_font, busy_icon, busy_icon_width, busy_image_height)
  {
    // start up the EPD
    epd_init(EPD_OPTIONS_DEFAULT);

#ifndef CONFIG_EPD_BOARD_REVISION_LILYGO_T5_47
    epd_poweron();
#endif
  }
  ~EpdiyRenderer()
  {
    epd_deinit();
  }
  void flush_display()
  {
    epd_hl_update_screen(&m_hl, needs_gray_flush ? MODE_GC16 : MODE_DU, temperature);
    needs_gray_flush = false;
  }
  void flush_area(int x, int y, int width, int height)
  {
    epd_hl_update_area(&m_hl, MODE_DU, temperature, {.x = x, .y = y, .width = width, .height = height});
  }
  virtual void reset()
  {
    ESP_LOGI("EPD", "Full clear");
    epd_fullclear(&m_hl, temperature);
  };
};