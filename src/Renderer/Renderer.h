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
  // temp buffer for measuring and rendering text - assumes that we don't have any words longer than
  // MAX_WORD_LENGTH
  char buffer[MAX_WORD_LENGTH];

  // helper function to get text from the src
  void get_text(const char *src, int start_index, int end_index);

public:
  ~Renderer();
  virtual void draw_image(const std::string &filename, int x, int y, int width, int height);
  virtual bool get_image_size(const std::string &filename, int *width, int *height);
  virtual void draw_pixel(int x, int y, uint8_t color) = 0;
  virtual int get_text_width(const char *src, int start_index, int end_index, bool italic = false, bool bold = false) = 0;
  virtual void draw_text(int x, int y, const char *src, int start_index, int end_index, bool italic = false, bool bold = false) = 0;
  virtual void draw_text_box(const std::string &text, int x, int y, int width, int height);
  virtual void draw_rect(int x, int y, int width, int height, uint8_t color = 0) = 0;
  virtual void clear_screen() = 0;
  virtual int get_page_width() = 0;
  virtual int get_page_height() = 0;
  virtual int get_space_width() = 0;
  virtual int get_line_height() = 0;
};