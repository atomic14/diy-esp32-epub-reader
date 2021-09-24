#pragma once
#include "Renderer.h"

class ConsoleRenderer : public Renderer
{
private:
  int y_cursor = 0;
  int x_cursor = 0;

public:
  ConsoleRenderer() {}
  ~ConsoleRenderer() {}
  int get_text_width(const std::string &text, bool bold = false, bool italic = false)
  {
    return text.length();
  }
  void draw_text(int x, int y, const std::string &text, bool bold = false, bool italic = false)
  {
    if (y_cursor < y)
    {
      for (int i = y_cursor; i < y; i++)
      {
        printf("\n");
      }
      y_cursor = y;
    }
    if (x_cursor < x)
    {
      for (int i = x_cursor; i < x; i++)
      {
        printf(" ");
      }
      x_cursor = x;
    }
    // close approximation to starting a new line
    if (x < x_cursor)
    {
      y_cursor++;
      x_cursor = 0;
    }
    printf("%s", text.c_str());
    x_cursor += text.length();
  }
  void draw_rect(int x, int y, int width, int height, uint8_t color = 0)
  {
    // nop
  }
  virtual void draw_pixel(int x, int y, uint8_t color)
  {
    //nop
  }
  virtual void draw_image(const char *filename, int x, int y, int width, int height)
  {
    printf("[%s]\n", filename);
  }
  virtual void clear_screen()
  {
    //nop
  }
  virtual int get_page_width()
  {
    return 40;
  }
  virtual int get_page_height()
  {
    return 20;
  }
  virtual int get_space_width()
  {
    return 1;
  }
  virtual int get_line_height()
  {
    return 1;
  }
};