#pragma once

#include <tjpgd.h>
#include "ImageHelper.h"

#define POOL_SIZE 32768

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
  JPEGHelper(const std::string &filename) : m_filename(filename) {}
  bool get_size(int *width, int *height)
  {
    ESP_LOGI("IMG", "Getting size of %s", m_filename.c_str());
    void *pool = malloc(POOL_SIZE);
    if (!pool)
    {
      ESP_LOGE("IMG", "Failed to allocate memory for pool");
      return false;
    }
    fp = fopen(m_filename.c_str(), "rb");
    if (!fp)
    {
      ESP_LOGE("IMG", "File not found: %s", m_filename.c_str());
      return false;
    }
    // decode the jpeg and get its size
    JDEC dec;
    JRESULT res = jd_prepare(&dec, read_jpeg_data, pool, POOL_SIZE, this);
    if (res == JDR_OK)
    {
      ESP_LOGI("IMG", "JPEG Decoded - size %d,%d", dec.width, dec.height);
      *width = dec.width;
      *height = dec.height;
    }
    else
    {
      ESP_LOGE("IMG", "JPEG Decode failed - %d", res);
      return false;
    }
    free(pool);
    fclose(fp);
    fp = NULL;
    return true;
  }
  bool render(Renderer *renderer, int x_pos, int y_pos, int width, int height, int scale_factor)
  {
    this->renderer = renderer;
    this->y_pos = y_pos;
    this->x_pos = x_pos;
    void *pool = malloc(POOL_SIZE);
    if (!pool)
    {
      ESP_LOGE("IMG", "Failed to allocate memory for pool");
      return false;
    }
    fp = fopen(m_filename.c_str(), "rb");
    if (!fp)
    {
      ESP_LOGE("IMG", "File not found: %s", m_filename.c_str());
      free(pool);
      return false;
    }
    // decode the jpeg and get its size
    JDEC dec;
    JRESULT res = jd_prepare(&dec, read_jpeg_data, pool, POOL_SIZE, this);
    if (res == JDR_OK)
    {
      ESP_LOGI("IMG", "JPEG Decoded - size %d,%d", dec.width, dec.height);
      jd_decomp(&dec, draw_jpeg_function, scale_factor);
    }
    else
    {
      ESP_LOGE("IMG", "JPEG Decode failed - %d", res);
    }
    free(pool);
    fclose(fp);
    fp = NULL;
    return res == JDR_OK;
  }
};

size_t read_jpeg_data(
    JDEC *jdec,    /* Pointer to the decompression object */
    uint8_t *buff, /* Pointer to buffer to store the read data */
    size_t ndata   /* Number of bytes to read/remove */
)
{
  JPEGHelper *context = (JPEGHelper *)jdec->device;
  FILE *fp = context->fp;
  if (!fp)
  {
    ESP_LOGE("IMG", "File is not open");
    return 0;
  }
  if (buff)
  {
    fread(buff, 1, ndata, fp);
  }
  else
  {
    fseek(fp, ndata, SEEK_CUR);
  }
  vTaskDelay(1);
  return ndata;
}

int draw_jpeg_function(
    JDEC *jdec,   /* Pointer to the decompression object */
    void *bitmap, /* Bitmap to be output */
    JRECT *rect   /* Rectangular region to output */
)
{
  JPEGHelper *context = (JPEGHelper *)jdec->device;
  Renderer *renderer = (Renderer *)context->renderer;
  uint8_t *grey = (uint8_t *)bitmap;
  for (int y = rect->top; y <= rect->bottom; y++)
  {
    for (int x = rect->left; x <= rect->right; x++)
    {
      renderer->draw_pixel(x + context->x_pos, y + context->y_pos, *grey);
      grey++;
    }
  }
  return 1;
}
