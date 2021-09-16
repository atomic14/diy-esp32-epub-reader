#pragma once

#include <tjpgd.h>
#include <string>
#include "ImageHelper.h"

size_t read_jpeg_data(
    JDEC *jdec,    /* Pointer to the decompression object */
    uint8_t *buff, /* Pointer to buffer to store the read data */
    size_t ndata   /* Number of bytes to read/remove */
);

int draw_jpeg_function(
    JDEC *jdec,   /* Pointer to the decompression object */
    void *bitmap, /* Bitmap to be output */
    JRECT *rect   /* Rectangular region to output */
);

class JPEGHelper : public ImageHelper
{
private:
  std::string m_filename;
  int scale_factor;
  // temporary vars used for the JPEG callbacks
  FILE *fp;
  Renderer *renderer;
  int x_pos;
  int y_pos;

  friend size_t read_jpeg_data(
      JDEC *jdec,    /* Pointer to the decompression object */
      uint8_t *buff, /* Pointer to buffer to store the read data */
      size_t ndata   /* Number of bytes to read/remove */
  );

  friend int draw_jpeg_function(
      JDEC *jdec,   /* Pointer to the decompression object */
      void *bitmap, /* Bitmap to be output */
      JRECT *rect   /* Rectangular region to output */
  );

public:
  bool get_size(const std::string &path, int *width, int *height, int max_width, int max_height);
  bool render(const std::string &path, Renderer *renderer, int x_pos, int y_pos, int width, int height);
};
