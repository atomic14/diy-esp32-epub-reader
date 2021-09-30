#pragma once

#include <string>

class Renderer;

class ImageHelper
{
public:
  virtual ~ImageHelper(){};
  virtual bool get_size(const uint8_t *data, size_t data_size, int *width, int *height) = 0;
  virtual bool render(const uint8_t *data, size_t data_size, Renderer *renderer, int x_pos, int y_pos, int width, int height) = 0;
};
