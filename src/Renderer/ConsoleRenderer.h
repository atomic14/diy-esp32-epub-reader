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
  int get_text_width(const char *src, int start_index, int end_index, bool italic = false, bool bold = false)
  {
    return end_index - start_index;
  }
  void draw_text(int x, int y, const char *src, int start_index, int end_index, bool italic = false, bool bold = false)
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
    for (int i = start_index; i < end_index; i++)
    {
      x_cursor++;
      printf("%c", src[i]);
    }
  }
  void draw_rect(int x, int y, int width, int height)
  {
    // nop
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