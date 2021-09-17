#pragma once
#include <esp_log.h>
#include <epd_driver.h>
#include <epd_highlevel.h>
#include "Renderer.h"
#include "epdiy_ED047TC1.h"

class EpdRenderer : public Renderer
{
private:
  const EpdFont *m_regular_font;
  const EpdFont *m_bold_font;
  const EpdFont *m_italic_font;
  const EpdFont *m_bold_italic_font;
  EpdiyHighlevelState m_hl;
  uint8_t *m_frame_buffer;
  EpdFontProperties m_font_props;

public:
  EpdRenderer(const EpdFont *regular_font, const EpdFont *bold_font, const EpdFont *italic_font, const EpdFont *bold_italic_font)
      : m_regular_font(regular_font), m_bold_font(bold_font), m_italic_font(italic_font), m_bold_italic_font(bold_italic_font)
  {
    m_font_props = epd_font_properties_default();
    // start up the EPD
    epd_init(EPD_OPTIONS_DEFAULT);
    m_hl = epd_hl_init(&epdiy_ED047TC1);
    // first set full screen to white
    epd_hl_set_all_white(&m_hl);
    epd_set_rotation(EPD_ROT_INVERTED_PORTRAIT);
    m_frame_buffer = epd_hl_get_framebuffer(&m_hl);
    epd_clear();
  }
  ~EpdRenderer() {}
  int get_text_width(const char *src, int start_index, int end_index, bool italic = false, bool bold = false)
  {
    get_text(src, start_index, end_index);
    int x = 0, y = 0, x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    epd_get_text_bounds(m_regular_font, buffer, &x, &y, &x1, &y1, &x2, &y2, &m_font_props);
    return x2 - x1;
  }
  void draw_text(int x, int y, const char *src, int start_index, int end_index, bool italic = false, bool bold = false)
  {
    get_text(src, start_index, end_index);
    int ypos = y + get_line_height();
    epd_write_string(m_regular_font, buffer, &x, &ypos, m_frame_buffer, &m_font_props);
  }
  void draw_rect(int x, int y, int width, int height, uint8_t color = 0)
  {
    epd_draw_rect({.x = x, .y = y, .width = width, .height = height}, color, m_frame_buffer);
  }
  virtual void draw_pixel(int x, int y, uint8_t color)
  {
    epd_draw_pixel(x, y, color, m_frame_buffer);
  }
  void clear_display()
  {
    epd_hl_set_all_white(&m_hl);
  }
  void flush_display()
  {
    ESP_LOGI("EPD", "Flushing display");
    epd_poweron();
    // ESP_LOGI(TAG, "epd_ambient_temperature=%f", epd_ambient_temperature());
    epd_hl_update_screen(&m_hl, MODE_GC16, 20);
    // vTaskDelay(50);
    epd_poweroff();
  }
  virtual void clear_screen()
  {
    epd_hl_set_all_white(&m_hl);
  }

  virtual int get_page_width()
  {
    // don't forget we are rotated
    return EPD_HEIGHT;
  }
  virtual int get_page_height()
  {
    // don't forget we are rotated
    return EPD_WIDTH
  }
  virtual int get_space_width()
  {
    auto space_glyph = epd_get_glyph(m_regular_font, ' ', m_font_props);
    return space_glyph->width;
  }
  virtual int get_line_height()
  {
    return m_regular_font->advance_y;
  }
};