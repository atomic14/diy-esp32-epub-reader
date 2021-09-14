#pragma once

class ImageHelper
{
public:
  virtual bool get_size(int *width, int *height) = 0;
  virtual bool render(Renderer *renderer, int x_pos, int y_pos, int width, int height, int scale_factor) = 0;
};
