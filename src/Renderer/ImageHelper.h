#pragma once

class ImageHelper
{
public:
  virtual bool get_size(const std::string &path, int *width, int *height, int max_width, int max_height) = 0;
  virtual bool render(const std::string &path, Renderer *renderer, int x_pos, int y_pos, int width, int height) = 0;
};
