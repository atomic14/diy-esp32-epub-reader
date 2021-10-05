#pragma once
#include <esp_log.h>
#include <epd_driver.h>
#include <epd_highlevel.h>
#include <math.h>
#include "Renderer.h"
#include "miniz.h"

#define GAMMA_VALUE (1.0f / 0.8f)

class EpdRenderer : public Renderer
{
private:
  const EpdFont *m_regular_font;
  const EpdFont *m_bold_font;
  const EpdFont *m_italic_font;
  const EpdFont *m_bold_italic_font;
  const uint8_t *m_busy_image;
  int m_busy_image_width;
  int m_busy_image_height;
  EpdiyHighlevelState m_hl;
  uint8_t *m_frame_buffer;
  EpdFontProperties m_font_props;
  uint8_t gamma_curve[256] = {0};
  bool needs_gray_flush = false;

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
  EpdRenderer(
      const EpdFont *regular_font,
      const EpdFont *bold_font,
      const EpdFont *italic_font,
      const EpdFont *bold_italic_font,
      const uint8_t *busy_icon,
      int busy_icon_width,
      int busy_icon_height)
      : m_regular_font(regular_font), m_bold_font(bold_font), m_italic_font(italic_font), m_bold_italic_font(bold_italic_font),
        m_busy_image(busy_icon), m_busy_image_width(busy_icon_width), m_busy_image_height(busy_icon_height)
  {
    m_font_props = epd_font_properties_default();
    // fallback to a question mark for character not available in the font
    m_font_props.fallback_glyph = '?';
    // start up the EPD
    epd_init(EPD_OPTIONS_DEFAULT);
    m_hl = epd_hl_init(EPD_BUILTIN_WAVEFORM);
    // first set full screen to white
    epd_hl_set_all_white(&m_hl);
    epd_set_rotation(EPD_ROT_INVERTED_PORTRAIT);
    m_frame_buffer = epd_hl_get_framebuffer(&m_hl);

    for (int gray_value = 0; gray_value < 256; gray_value++)
    {
      gamma_curve[gray_value] = round(255 * pow(gray_value / 255.0, GAMMA_VALUE));
    }
  }
  ~EpdRenderer()
  {
    epd_deinit();
  }
  void show_busy()
  {
    epd_draw_rotated_transparent_image(
        {.x = (EPD_HEIGHT - m_busy_image_width) / 2,
         .y = (EPD_WIDTH - m_busy_image_height) / 2,
         // don't forget we're rotated...
         .width = m_busy_image_width,
         .height = m_busy_image_height},
        m_busy_image, m_frame_buffer,
        0xE0);
    needs_gray_flush = true;
    flush_display();
  }
  int get_text_width(const char *text, bool bold = false, bool italic = false)
  {
    int x = 0, y = 0, x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    epd_get_text_bounds(get_font(bold, italic), text, &x, &y, &x1, &y1, &x2, &y2, &m_font_props);
    return x2 - x1;
  }
  void draw_text(int x, int y, const char *text, bool bold = false, bool italic = false)
  {
    // if using antialised text then set to gray next flush
    // needs_gray_flush = true;
    int ypos = y + get_line_height() + margin_top;
    epd_write_string(get_font(bold, italic), text, &x, &ypos, m_frame_buffer, &m_font_props);
  }
  void draw_rect(int x, int y, int width, int height, uint8_t color = 0)
  {
    if (color != 0 && color != 255)
    {
      needs_gray_flush = true;
    }
    epd_draw_rect({.x = x, .y = y + margin_top, .width = width, .height = height}, color, m_frame_buffer);
  }
  virtual void fill_rect(int x, int y, int width, int height, uint8_t color = 0)
  {
    if (color != 0 && color != 255)
    {
      needs_gray_flush = true;
    }
    epd_fill_rect({.x = x, .y = y + margin_top, .width = width, .height = height}, color, m_frame_buffer);
  }
  virtual void draw_pixel(int x, int y, uint8_t color)
  {
    uint8_t corrected_color = gamma_curve[color];
    if (corrected_color != 0 && corrected_color != 255)
    {
      needs_gray_flush = true;
    }
    epd_draw_pixel(x, y + margin_top, corrected_color, m_frame_buffer);
  }
  void flush_display()
  {
    epd_hl_update_screen(&m_hl, needs_gray_flush ? MODE_GC16 : MODE_DU, 20);
    needs_gray_flush = false;
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
    return EPD_WIDTH - margin_top;
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
    size_t front_buffer_compressed_size = 0;
    void *front_buffer_compressed = tdefl_compress_mem_to_heap(m_frame_buffer, EPD_WIDTH * EPD_HEIGHT / 2, &front_buffer_compressed_size, 0);
    if (front_buffer_compressed)
    {
      ESP_LOGI("EPD", "Front buffer compressed size: %d", front_buffer_compressed_size);
      FILE *front_fp = fopen("/fs/front_buffer.z", "w");
      if (front_fp)
      {
        int written = fwrite(front_buffer_compressed, 1, front_buffer_compressed_size, front_fp);
        if (written == 0)
        {
          // try again?
          ESP_LOGI("EPD", "Retrying front buffer save");
          written = fwrite(front_buffer_compressed, 1, front_buffer_compressed_size, front_fp);
        }
        fclose(front_fp);
        ESP_LOGI("EPD", "Front buffer saved %d", written);
      }
      free(front_buffer_compressed);
    }
    else
    {
      ESP_LOGE("EPD", "Failed to compress front buffer");
    }
    size_t back_buffer_compressed_size = 0;
    void *back_buffer_compressed = tdefl_compress_mem_to_heap(m_hl.back_fb, EPD_WIDTH * EPD_HEIGHT / 2, &back_buffer_compressed_size, 0);
    if (back_buffer_compressed)
    {
      ESP_LOGI("EPD", "Back buffer compressed size: %d", back_buffer_compressed_size);
      FILE *back_fp = fopen("/fs/back_buffer.z", "w");
      if (back_fp)
      {
        int written = fwrite(back_buffer_compressed, 1, back_buffer_compressed_size, back_fp);
        if (written == 0)
        {
          // try again?
          ESP_LOGI("EPD", "Retrying back buffer save");
          written = fwrite(back_buffer_compressed, 1, back_buffer_compressed_size, back_fp);
        }
        fclose(back_fp);
        ESP_LOGI("EPD", "Back buffer saved %d", written);
      }
    }
    else
    {
      ESP_LOGE("EPD", "Failed to compress back buffer");
    }
    ESP_LOGI("EPD", "Dehydrated EPD");
  };
  // deep sleep helper - retrieve any state from disk after wake
  virtual void hydrate()
  {
    ESP_LOGI("EPD", "Hydrating EPD");
    // load the two buffers - the front and the back buffers
    FILE *fp = fopen("/fs/front_buffer.z", "r");
    if (fp)
    {
      fseek(fp, 0, SEEK_END);
      size_t compressed_size = ftell(fp);
      ESP_LOGI("EPD", "Front buffer compressed size: %d", compressed_size);
      fseek(fp, 0, SEEK_SET);
      void *compressed = malloc(compressed_size);
      if (compressed)
      {
        fread(compressed, 1, compressed_size, fp);
        int result = tinfl_decompress_mem_to_mem(m_hl.front_fb, EPD_HEIGHT * EPD_WIDTH / 2, compressed, compressed_size, 0);
        if (result == TINFL_DECOMPRESS_MEM_TO_MEM_FAILED)
        {
          ESP_LOGE("EPD", "Failed to decompress front buffer");
        }
        free(compressed);
      }
      else
      {
        ESP_LOGE("EPD", "Failed to allocate memory for front buffer");
      }
      fclose(fp);
    }
    else
    {
      ESP_LOGI("EPD", "No front buffer found");
      reset();
      return;
    }
    fp = fopen("/fs/back_buffer.z", "r");
    if (fp)
    {
      fseek(fp, 0, SEEK_END);
      size_t compressed_size = ftell(fp);
      fseek(fp, 0, SEEK_SET);
      ESP_LOGI("EPD", "Back buffer compressed size: %d", compressed_size);
      void *compressed = malloc(compressed_size);
      if (compressed)
      {
        fread(compressed, 1, compressed_size, fp);
        int result = tinfl_decompress_mem_to_mem(m_hl.back_fb, EPD_HEIGHT * EPD_WIDTH / 2, compressed, compressed_size, 0);
        if (result == TINFL_DECOMPRESS_MEM_TO_MEM_FAILED)
        {
          ESP_LOGE("EPD", "Failed to decompress back buffer");
        }
        free(compressed);
      }
      else
      {
        ESP_LOGE("EPD", "Failed to allocate memory for back buffer");
      }
      fclose(fp);
    }
    else
    {
      ESP_LOGI("EPD", "No back buffer found");
      reset();
      return;
    }
    ESP_LOGI("EPD", "Hydrated EPD");
  };
  virtual void reset()
  {
    epd_fullclear(&m_hl, 20);
  };
};