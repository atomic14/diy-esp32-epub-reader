#pragma once
#include <esp_log.h>
#include <epd_driver.h>
#include <epd_highlevel.h>
#include <math.h>
#include "Renderer.h"
#include "miniz.h"

#define GAMMA_VALUE (1.0f / 0.8f)

class EpdiyFrameBufferRenderer : public Renderer
{
protected:
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
  EpdiyFrameBufferRenderer(
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
  virtual ~EpdiyFrameBufferRenderer()
  {
  }
  void show_busy()
  {
    int x = (EPD_HEIGHT - m_busy_image_width) / 2;
    int y = (EPD_WIDTH - m_busy_image_height) / 2;
    int width = m_busy_image_width;
    int height = m_busy_image_height;
    EpdRect image_area = {.x = x,
                          .y = y,
                          // don't forget we're rotated...
                          .width = width,
                          .height = height};
    epd_draw_rotated_transparent_image(
        image_area,
        m_busy_image, m_frame_buffer,
        0xE0);
    needs_gray_flush = true;
    flush_area(x, y, width, height);
  }

  void needs_gray(uint8_t color)
  {
    if (color != 0 && color != 255)
    {
      needs_gray_flush = true;
    }
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
    int xpos = x + margin_left;
    epd_write_string(get_font(bold, italic), text, &xpos, &ypos, m_frame_buffer, &m_font_props);
  }
  void draw_rect(int x, int y, int width, int height, uint8_t color = 0)
  {
    needs_gray(color);
    epd_draw_rect({.x = x + margin_left, .y = y + margin_top, .width = width, .height = height}, color, m_frame_buffer);
  }
  virtual void fill_rect(int x, int y, int width, int height, uint8_t color = 0)
  {
    needs_gray(color);
    epd_fill_rect({.x = x + margin_left, .y = y + margin_top, .width = width, .height = height}, color, m_frame_buffer);
  }
  virtual void fill_circle(int x, int y, int r, uint8_t color = 0)
  {
    needs_gray(color);
    epd_fill_circle(x, y, r, color, m_frame_buffer);
  }
  virtual void fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color)
  {
    needs_gray(color);
    epd_fill_triangle(x0, y0, x1, y1, x2, y2, color, m_frame_buffer);
  }
  virtual void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color)
  {
    needs_gray(color);
    epd_draw_triangle(x0, y0, x1, y1, x2, y2, color, m_frame_buffer);
  }
  virtual void draw_pixel(int x, int y, uint8_t color)
  {
    uint8_t corrected_color = gamma_curve[color];
    needs_gray(corrected_color);
    epd_draw_pixel(x + margin_left, y + margin_top, corrected_color, m_frame_buffer);
  }
  virtual void draw_circle(int x, int y, int r, uint8_t color = 0)
  {
    needs_gray(color);
    epd_draw_circle(x, y, r, color, m_frame_buffer);
  }
  virtual void flush_display() = 0;
  virtual void flush_area(int x, int y, int width, int height) = 0;

  virtual void clear_screen()
  {
    epd_hl_set_all_white(&m_hl);
  }
  virtual int get_page_width()
  {
    // don't forget we are rotated
    return EPD_HEIGHT - (margin_left + margin_right);
  }
  virtual int get_page_height()
  {
    // don't forget we are rotated
    return EPD_WIDTH - (margin_top + margin_bottom);
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

  // dehydate a frame buffer to file
  bool dehydrate_frame_buffer(const char *fname, uint8_t *buffer, size_t size)
  {
    // compress the buffer to save space and increase performance - writing data is slow!
    size_t compressed_size = 0;
    void *compressed = tdefl_compress_mem_to_heap(m_frame_buffer, EPD_WIDTH * EPD_HEIGHT / 2, &compressed_size, 0);
    if (compressed)
    {
      ESP_LOGI("EPD", "Buffer compressed size: %d", compressed_size);
      FILE *fp = fopen(fname, "w");
      if (fp)
      {
        int written = fwrite(compressed, 1, compressed_size, fp);
        if (written == 0)
        {
          // try again?
          ESP_LOGI("EPD", "0 bytes written - retrying buffer save");
          written = fwrite(compressed, 1, compressed_size, fp);
        }
        fclose(fp);
        ESP_LOGI("EPD", "Buffer saved %d", written);
      }
      free(compressed);
      return true;
    }
    return false;
  }

  // deep sleep helper - persist any state to disk that may be needed on wake
  virtual void dehydrate()
  {
    ESP_LOGI("EPD", "Dehydrating EPD");
    // only need to save the front buffer - it should be exactly the same as the back buffer
    if (!dehydrate_frame_buffer("/fs/front_buffer.z", m_frame_buffer, EPD_WIDTH * EPD_HEIGHT / 2))
    {
      ESP_LOGI("EPD", "Failed to save front buffer");
    }
    ESP_LOGI("EPD", "Dehydrated EPD");
  };
  // hydrate a frame buffer
  bool hydrate_frame_buffer(const char *fname, uint8_t *buffer, size_t size)
  {
    // load the two buffers - the front and the back buffers
    FILE *fp = fopen(fname, "r");
    bool success = false;
    if (fp)
    {
      fseek(fp, 0, SEEK_END);
      size_t compressed_size = ftell(fp);
      fseek(fp, 0, SEEK_SET);
      if (compressed_size > 0)
      {
        ESP_LOGI("EPD", "Buffer compressed size: %d", compressed_size);
        void *compressed = malloc(compressed_size);
        if (compressed)
        {
          fread(compressed, 1, compressed_size, fp);
          int result = tinfl_decompress_mem_to_mem(buffer, size, compressed, compressed_size, 0);
          if (result == TINFL_DECOMPRESS_MEM_TO_MEM_FAILED)
          {
            ESP_LOGE("EPD", "Failed to decompress front buffer");
          }
          else
          {
            success = true;
            ESP_LOGI("EPD", "Success decompressing %d bytes", size);
          }
          free(compressed);
        }
        else
        {
          ESP_LOGE("EPD", "Failed to allocate memory for front buffer");
        }
      }
      else
      {
        ESP_LOGE("EPD", "No data to restore");
      }
      fclose(fp);
    }
    else
    {
      ESP_LOGI("EPD", "No front buffer found");
    }
    return success;
  }

  // deep sleep helper - retrieve any state from disk after wake
  virtual bool hydrate()
  {
    ESP_LOGI("EPD", "Hydrating EPD");
    if (hydrate_frame_buffer("/fs/front_buffer.z", m_frame_buffer, EPD_WIDTH * EPD_HEIGHT / 2))
    {
      // just memcopy the front buffer to the back buffer - they should be exactly the same
      memcpy(m_hl.back_fb, m_frame_buffer, EPD_WIDTH * EPD_HEIGHT / 2);
      ESP_LOGI("EPD", "Hydrated EPD");
      return true;
    }
    else
    {
      ESP_LOGI("EPD", "Hydrate EPD failed");
      reset();
      return false;
    }
  };
  virtual void reset() = 0;
};