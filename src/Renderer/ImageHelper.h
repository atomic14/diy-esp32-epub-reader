#pragma once

#include <string>

class Renderer;

class ImageHelper
{
public:
  virtual ~ImageHelper(){};
  virtual bool get_size(const std::string &path, int *width, int *height) = 0;
  virtual bool render(const std::string &path, Renderer *renderer, int x_pos, int y_pos, int width, int height) = 0;
};
