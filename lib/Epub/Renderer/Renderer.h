#pragma once

#include <string>

class ImageHelper;

#define MAX_WORD_LENGTH 100

class Renderer
{
private:
  ImageHelper *png_helper = nullptr;
  ImageHelper *jpeg_helper = nullptr;

  ImageHelper *get_image_helper(const std::string &filename, const uint8_t *data, size_t data_size);

protected:
  int margin_top = 0;
  int margin_bottom = 0;
  int margin_left = 0;
  int margin_right = 0;

public:
  virtual ~Renderer();
  virtual void draw_image(const std::string &filename, const uint8_t *data, size_t data_size, int x, int y, int width, int height);
  virtual bool get_image_size(const std::string &filename, const uint8_t *data, size_t data_size, int *width, int *height);
  virtual void draw_pixel(int x, int y, uint8_t color) = 0;
  virtual int get_text_width(const char *text, bool bold = false, bool italic = false) = 0;
  virtual void draw_text(int x, int y, const char *text, bool bold = false, bool italic = false) = 0;
  virtual void draw_text_box(const std::string &text, int x, int y, int width, int height, bool bold = false, bool italic = false);
  virtual void draw_rect(int x, int y, int width, int height, uint8_t color = 0) = 0;
  virtual void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color) = 0;
  virtual void draw_circle(int x, int y, int r, uint8_t color = 0) = 0;

  virtual void fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color) = 0;
  virtual void fill_rect(int x, int y, int width, int height, uint8_t color = 0) = 0;
  virtual void fill_circle(int x, int y, int r, uint8_t color = 0) = 0;
  virtual void needs_gray(uint8_t color) = 0;
  virtual void show_busy() = 0;
  virtual void clear_screen() = 0;
  virtual void flush_display(){};
  virtual void flush_area(int x, int y, int width, int height){};

  virtual int get_page_width() = 0;
  virtual int get_page_height() = 0;
  virtual int get_space_width() = 0;
  virtual int get_line_height() = 0;
  // set margins
  void set_margin_top(int margin_top) { this->margin_top = margin_top; }
  void set_margin_bottom(int margin_bottom) { this->margin_bottom = margin_bottom; }
  void set_margin_left(int margin_left) { this->margin_left = margin_left; }
  void set_margin_right(int margin_right) { this->margin_right = margin_right; }
  // deep sleep helper - persist any state to disk that may be needed on wake
  virtual bool dehydrate() { return false; };
  // deep sleep helper - retrieve any state from disk after wake
  virtual bool hydrate() { return false; };
  // really really clear the screen
  virtual void reset(){};

  uint8_t temperature = 20;
};