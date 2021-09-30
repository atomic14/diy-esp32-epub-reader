#pragma once

#include <string>
#include <PNGdec.h>
#include "ImageHelper.h"

class Renderer;

void png_draw_callback(PNGDRAW *draw);

class PNGHelper : public ImageHelper
{
private:
  // temporary vars used for the PNG callbacks
  Renderer *renderer;
  int x_pos;
  int y_pos;
  int last_y;
  float x_scale;
  float y_scale;
  uint16_t *tmp_rgb565_buffer;

  PNG png;

  friend void png_draw_callback(PNGDRAW *draw);

public:
  bool get_size(const uint8_t *data, size_t data_size, int *width, int *height);
  bool render(const uint8_t *data, size_t data_size, Renderer *renderer, int x_pos, int y_pos, int width, int height);
  void draw_callback(PNGDRAW *draw);
};