#pragma once

#include <string>
#include <PNGdec.h>
#include "ImageHelper.h"

class Renderer;

void png_draw_callback(PNGDRAW *draw);

class PNGHelper : public ImageHelper
{
private:
  std::string m_filename;
  // temporary vars used for the PNG callbacks
  Renderer *renderer;
  int x_pos;
  int y_pos;
  int scale_factor;
  float scale;
  int current_y = 0;
  uint16_t *tmp_rgb565_buffer;
  int *accumulation_buffer;
  int *count_buffer;

  PNG png;

  friend void png_draw_callback(PNGDRAW *draw);

public:
  bool get_size(const std::string &path, int *width, int *height, int max_width, int max_height);
  bool render(const std::string &path, Renderer *renderer, int x_pos, int y_pos, int width, int height);
  void draw_callback(PNGDRAW *draw);
};