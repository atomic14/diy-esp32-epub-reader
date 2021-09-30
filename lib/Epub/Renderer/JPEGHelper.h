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
  float x_scale;
  float y_scale;
  int scale_factor;
  // temporary vars used for the JPEG callbacks
  const uint8_t *m_data;
  size_t m_data_pos;

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
  bool get_size(const uint8_t *data, size_t data_size, int *width, int *height);
  bool render(const uint8_t *data, size_t data_size, Renderer *renderer, int x_pos, int y_pos, int width, int height);
};
