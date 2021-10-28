#pragma once
#include <esp_log.h>
#include <M5EPD_Driver.h>
#include <driver/rtc_io.h>
#include <driver/gpio.h>
#include "EpdiyFrameBufferRenderer.h"
#include "miniz.h"

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
    driver.begin();
    driver.SetColorReverse(true);

    m_frame_buffer = (uint8_t *)malloc(EPD_WIDTH * EPD_HEIGHT / 2);
    clear_screen();
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
    // there's probably a way of only sending the data we need to send for the area
    driver.WriteFullGram4bpp(m_frame_buffer);
    // don't forger we're rotated
    driver.UpdateArea(y, x, height, width, needs_gray_flush ? UPDATE_MODE_GC16 : UPDATE_MODE_DU);
    needs_gray_flush = false;
  }
  virtual bool hydrate()
  {
    ESP_LOGI("M5P", "Hydrating EPD");
    if (EpdiyFrameBufferRenderer::hydrate())
    {
      ESP_LOGI("M5P", "Hydrated EPD");
      driver.WriteFullGram4bpp(m_frame_buffer);
      driver.UpdateFull(UPDATE_MODE_GC16);
      return true;
    }
    else
    {
      ESP_LOGI("M5P", "Hydrate EPD failed");
      reset();
      return false;
    }
  }
  virtual void reset()
  {
    ESP_LOGI("M5P", "Full clear");
    clear_screen();
    // flushing to white
    needs_gray_flush = false;
    flush_display();
  };
};