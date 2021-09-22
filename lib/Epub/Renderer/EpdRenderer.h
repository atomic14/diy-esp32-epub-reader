#pragma once
#include <esp_log.h>
#include <epd_driver.h>
#include <epd_highlevel.h>
#include <math.h>
#include "Renderer.h"
#include "miniz.h"

#define GAMMA_VALUE (1.0f / 0.9f)

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
  uint8_t gamma_curve[256] = {0};

  const EpdFont *get_font(bool is_bold, bool is_italic)
  {
    if (is_bold && is_italic)
    {
      return m_bold_italic_font;
    }
    if (is_bold)
    {
      return m_bold_font;
    }
    if (is_italic)
    {
      return m_italic_font;
    }
    return m_regular_font;
  }

public:
  EpdRenderer(const EpdFont *regular_font, const EpdFont *bold_font, const EpdFont *italic_font, const EpdFont *bold_italic_font)
      : m_regular_font(regular_font), m_bold_font(bold_font), m_italic_font(italic_font), m_bold_italic_font(bold_italic_font)
  {
    m_font_props = epd_font_properties_default();
    // start up the EPD
    epd_init(EPD_OPTIONS_DEFAULT);
    m_hl = epd_hl_init(EPD_BUILTIN_WAVEFORM);
    // first set full screen to white
    epd_hl_set_all_white(&m_hl);
    epd_set_rotation(EPD_ROT_INVERTED_PORTRAIT);
    m_frame_buffer = epd_hl_get_framebuffer(&m_hl);
    
    for (int gray_value =0; gray_value<256;gray_value++)
    gamma_curve[gray_value] = round (255*pow(gray_value/255.0, GAMMA_VALUE));
  }
  ~EpdRenderer()
  {
    epd_deinit();
  }
  int get_text_width(const char *src, int start_index, int end_index, bool bold = false, bool italic = false)
  {
    get_text(src, start_index, end_index);
    int x = 0, y = 0, x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    epd_get_text_bounds(get_font(bold, italic), buffer, &x, &y, &x1, &y1, &x2, &y2, &m_font_props);
    return x2 - x1;
  }
  void draw_text(int x, int y, const char *src, int start_index, int end_index, bool bold = false, bool italic = false)
  {
    get_text(src, start_index, end_index);
    int ypos = y + get_line_height();
    epd_write_string(get_font(bold, italic), buffer, &x, &ypos, m_frame_buffer, &m_font_props);
  }
  void draw_rect(int x, int y, int width, int height, uint8_t color = 0)
  {
    epd_draw_rect({.x = x, .y = y, .width = width, .height = height}, color, m_frame_buffer);
  }
  virtual void draw_pixel(int x, int y, uint8_t color)
  {
    epd_draw_pixel(x, y, gamma_curve[color], m_frame_buffer);
  }
  void flush_display(bool is_grey_scale = true)
  {
    ESP_LOGI("EPD", "Flushing display");
    epd_poweron();
    epd_hl_update_screen(&m_hl, is_grey_scale ? MODE_GC16 : MODE_DU, 20);
    // vTaskDelay(50);
    epd_poweroff();
    ESP_LOGI("EPD", "Flushed display");
  }
  virtual void clear_screen()
  {
    epd_hl_set_all_white(&m_hl);
    epd_clear();
  }
  virtual int get_page_width()
  {
    // don't forget we are rotated
    return EPD_HEIGHT;
  }
  virtual int get_page_height()
  {
    // don't forget we are rotated
    return EPD_WIDTH;
  }
  virtual int get_space_width()
  {
    auto space_glyph = epd_get_glyph(m_regular_font, ' ');
    return space_glyph->advance_x;
  }
  virtual int get_line_height()
  {
    return m_regular_font->advance_y;
  }
  // deep sleep helper - persist any state to disk that may be needed on wake
  virtual void dehydrate()
  {
    ESP_LOGI("EPD", "Dehydrating EPD");
    // save the two buffers - the front and the back buffers
    size_t compressed_size = 0;
    void *compressed = tdefl_compress_mem_to_heap(m_frame_buffer, EPD_WIDTH * EPD_HEIGHT/2, &compressed_size, 0);
    if (compressed) {
      ESP_LOGI("EPD", "Front buffer compressed size: %d", compressed_size);
      FILE *fp = fopen("/sdcard/front_buffer.z", "wb");
      if (fp)
      {
        fwrite(compressed, 1, compressed_size, fp);
        fclose(fp);
      }
      free(compressed);
    } else {
      ESP_LOGE("EPD", "Failed to compress front buffer");
    }
    compressed_size = 0;
    compressed = tdefl_compress_mem_to_heap(m_hl.back_fb, EPD_WIDTH * EPD_HEIGHT/2, &compressed_size, 0);
    if (compressed) {
      ESP_LOGI("EPD", "Back buffer compressed size: %d", compressed_size);
      FILE *fp = fopen("/sdcard/back_buffer.z", "wb");
      if (fp)
      {
        fwrite(compressed, 1, compressed_size, fp);
        fclose(fp);
      }
    } else {
      ESP_LOGE("EPD", "Failed to compress back buffer");
    }
    ESP_LOGI("EPD", "Dehydrated EPD");
  };
  // deep sleep helper - retrieve any state from disk after wake
  virtual void hydrate()
  {
    ESP_LOGI("EPD", "Hydrating EPD");
    // load the two buffers - the front and the back buffers
    FILE *fp = fopen("/sdcard/front_buffer.z", "rb");
    if (fp)
    {
      fseek(fp, 0, SEEK_END);
      size_t compressed_size = ftell(fp);
      fseek(fp, 0, SEEK_SET);
      void *compressed = malloc(compressed_size);
      if (compressed) {
        fread(compressed, 1, compressed_size, fp);
        tinfl_decompress_mem_to_mem(m_hl.front_fb, EPD_HEIGHT * EPD_WIDTH / 2, compressed, compressed_size, 0);
        free(compressed);
      }
      fclose(fp);
    }
    else
    {
      ESP_LOGI("EPD", "No front buffer found");
    }
    fp = fopen("/sdcard/back_buffer.z", "rb");
    if (fp)
    {
      fseek(fp, 0, SEEK_END);
      size_t compressed_size = ftell(fp);
      fseek(fp, 0, SEEK_SET);
      void *compressed = malloc(compressed_size);
      if (compressed) {
        fread(compressed, 1, compressed_size, fp);
        tinfl_decompress_mem_to_mem(m_hl.back_fb, EPD_HEIGHT * EPD_WIDTH / 2, compressed, compressed_size, 0);
        free(compressed);
       }
       fclose(fp);
    }
    else
    {
      ESP_LOGI("EPD", "No back buffer found");
    }
    ESP_LOGI("EPD", "Hydrated EPD");
  };
};