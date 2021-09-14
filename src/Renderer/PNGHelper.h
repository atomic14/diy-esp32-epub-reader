#pragma once

#include <PNGdec.h>
#include "ImageHelper.h"

void *myOpen(const char *filename, int32_t *size)
{
  ESP_LOGI(TAG, "Attempting to open %s\n", filename);
  fp = fopen(filename, "rb");
  if (fp == NULL)
  {
    ESP_LOGE(TAG, "Failed to open %s\n", filename);
    return NULL;
  }
  fseek(fp, 0, SEEK_END);
  *size = ftell(fp);
  rewind(fp);
  ESP_LOGI(TAG, "Opened %s, size %d\n", filename, *size);
  return fp;
}
void myClose(void *handle)
{
  ESP_LOGI(TAG, "Closing file");
  fclose((FILE *)fp);
}

int32_t myRead(PNGFILE *handle, uint8_t *buffer, int32_t length)
{
  return fread(buffer, 1, length, (FILE *)(handle->fHandle));
}

int32_t mySeek(PNGFILE *handle, int32_t position)
{
  return fseek((FILE *)(handle->fHandle), position, SEEK_SET);
}

class PNGHelper
{
private:
  std::string m_filename;
  // temporary vars used for the JPEG callbacks
  Renderer *renderer;
  int x_pos;
  int y_pos;

public:
  PNGHelper(const std::string &filename) : m_filename(filename) {}
  bool get_size(int *width, int *height)
  {
    PNG png;
    ESP_LOGI("IMG", "Getting size of %s", m_filename.c_str());
    int rc = png.open(filename, myOpen, myClose, myRead, mySeek, NULL);
    if (rc == PNG_SUCCESS)
    {
      ESP_LOGI(TAG, "image specs: (%d x %d), %d bpp, pixel type: %d", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
      *width = png.getWidth();
      *height = png.getHeight();
      return true;
      png.close();
    }
    else
    {
      ESP_LOGE(TAG, "failed to open %s %d", filename, rc);
      return false;
    }
  }
  bool render(Renderer *renderer, int x_pos, int y_pos, int width, int height, int scale_factor)
  {
    PNG png;
    this->renderer = renderer;
    this->y_pos = y_pos;
    this->x_pos = x_pos;
    int rc = png.open(filename, myOpen, myClose, myRead, mySeek, png_draw_callback);
    if (rc == PNG_SUCCESS)
    {
      png.decode(this);
    }
    // 0 = 1 to 1
    // 1 = /2
    // 2 = /4
    // 3 = /8
    int width = png.getWidth();
    int height = png.getHeight();
    // buffer to hold the scaled line
    int out_buffer[width >> scale_factor] = {0};
    int16_t pixels[width];
    for (int)
      png.getLineAsRGB565(NULL, pixels, 0, -1);
  }
  else
  {
    ESP_LOGE(TAG, "failed to open %s %d", filename, rc);
    return false;
  }
}
}
;
