#pragma once

#include <string>

class ImageHelper;

#define MAX_WORD_LENGTH 100

class Renderer
{
private:
  ImageHelper *png_helper = nullptr;
  ImageHelper *jpeg_helper = nullptr;

  ImageHelper *get_image_helper(const std::string &filename);

protected:
  int margin_top = 0;

public:
  virtual ~Renderer();
  virtual void draw_image(const std::string &filename, int x, int y, int width, int height);
  virtual bool get_image_size(const std::string &filename, int *width, int *height);
  virtual void draw_pixel(int x, int y, uint8_t color) = 0;
  virtual int get_text_width(const char *text, bool bold = false, bool italic = false) = 0;
  virtual void draw_text(int x, int y, const char *text, bool bold = false, bool italic = false) = 0;
  virtual void draw_text_box(const std::string &text, int x, int y, int width, int height, bool bold = false, bool italic = false);
  virtual void draw_rect(int x, int y, int width, int height, uint8_t color = 0) = 0;
  virtual void fill_rect(int x, int y, int width, int height, uint8_t color = 0) = 0;
  virtual void show_busy() = 0;
  virtual void clear_screen() = 0;
  virtual void flush_display(){};
  virtual int get_page_width() = 0;
  virtual int get_page_height() = 0;
  virtual int get_space_width() = 0;
  virtual int get_line_height() = 0;
  // set the top margin
  void set_margin_top(int margin_top) { this->margin_top = margin_top; }
  // deep sleep helper - persist any state to disk that may be needed on wake
  virtual void dehydrate(){};
  // deep sleep helper - retrieve any state from disk after wake
  virtual void hydrate(){};
  // really really clear the screen
  virtual void reset(){};
};