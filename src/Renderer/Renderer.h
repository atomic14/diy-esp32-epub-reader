#pragma once

class Renderer
{
public:
  ~Renderer() {}
  virtual int get_text_width(const char *src, int start_index, int end_index, bool italic = false, bool bold = false) = 0;
  virtual void draw_text(int x, int y, const char *src, int start_index, int end_index, bool italic = false, bool bold = false) = 0;
  virtual void draw_rect(int x, int y, int width, int height) = 0;
  virtual void draw_pixel(int x, int y, uint8_t color) = 0;
  virtual int get_page_width() = 0;
  virtual int get_page_height() = 0;
  virtual int get_space_width() = 0;
  virtual int get_line_height() = 0;
};