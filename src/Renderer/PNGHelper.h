#pragma once

#include <PNGdec.h>
#include "ImageHelper.h"

void *myOpen(const char *filename, int32_t *size)
{
  ESP_LOGI(TAG, "Attempting to open %s\n", filename);
  FILE *fp = fopen(filename, "rb");
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
  fclose((FILE *)handle);
}

int32_t myRead(PNGFILE *png_file, uint8_t *buffer, int32_t length)
{
  return fread(buffer, 1, length, (FILE *)(png_file->fHandle));
}

int32_t mySeek(PNGFILE *png_file, int32_t position)
{
  return fseek((FILE *)(png_file->fHandle), position, SEEK_SET);
}

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
  bool get_size(const std::string &path, int *width, int *height, int max_width, int max_height)
  {
    ESP_LOGI("IMG", "Getting size of %s", m_filename.c_str());
    scale = 1;
    int rc = png.open(path.c_str(), myOpen, myClose, myRead, mySeek, NULL);
    if (rc == PNG_SUCCESS)
    {
      ESP_LOGI(TAG, "image specs: (%d x %d), %d bpp, pixel type: %d", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
      *width = png.getWidth();
      *height = png.getHeight();
      scale = std::min(1.0f, std::min(float(max_width) / float(*width), float(max_height) / float(*height)));
      *width = *width * scale;
      *height = *height * scale;
      return true;
      png.close();
    }
    else
    {
      ESP_LOGE(TAG, "failed to open %s %d", m_filename.c_str(), rc);
      return false;
    }
  }
  bool render(const std::string &path, Renderer *renderer, int x_pos, int y_pos, int width, int height)
  {
    this->renderer = renderer;
    this->y_pos = y_pos;
    this->x_pos = x_pos;
    this->current_y = 0;
    int rc = png.open(path.c_str(), myOpen, myClose, myRead, mySeek, png_draw_callback);
    if (rc == PNG_SUCCESS)
    {
      this->tmp_rgb565_buffer = (uint16_t *)malloc(png.getWidth() * 2);
      this->accumulation_buffer = (int *)malloc(sizeof(int) * png.getWidth() * scale);
      this->count_buffer = (int *)malloc(sizeof(int) * png.getWidth() * scale);
      memset(accumulation_buffer, 0, sizeof(int) * png.getWidth() * scale);
      memset(count_buffer, 0, sizeof(int) * png.getWidth() * scale);

      png.decode(this, 0);
      png.close();
      free(this->tmp_rgb565_buffer);
      free(this->accumulation_buffer);
      return true;
    }
    else
    {
      ESP_LOGE(TAG, "failed to open %s %d", m_filename.c_str(), rc);
      return false;
    }
  }
  void convert_rgb_565_to_rgb(uint16_t rgb565, uint8_t *r, uint8_t *g, uint8_t *b)
  {
    *r = ((rgb565 >> 11) & 0x1F) << 3;
    *g = ((rgb565 >> 5) & 0x3F) << 2;
    *b = (rgb565 & 0x1F) << 3;
  }
  void draw_callback(PNGDRAW *draw)
  {
    if (draw->y * scale > current_y)
    {
      // draw the average value from the accumulation buffer
      for (int i = 0; i < png.getWidth(); i++)
      {
        renderer->draw_pixel(i * scale, draw->y * scale, accumulation_buffer[int(i * scale)] / count_buffer[int(i * scale)]);
      }
      // clear the accumulation buffer
      memset(accumulation_buffer, 0, sizeof(int) * png.getWidth() * scale);
      memset(count_buffer, 0, sizeof(int) * png.getWidth() * scale);
      current_y = draw->y * scale;
    }
    // // get the rgb 565 pixel values
    png.getLineAsRGB565(draw, tmp_rgb565_buffer, 0, 0);
    // add the grayscale values to the accumulation buffer
    for (int i = 0; i < png.getWidth(); i++)
    {
      uint8_t r, g, b;
      convert_rgb_565_to_rgb(tmp_rgb565_buffer[i], &r, &g, &b);
      uint8_t gray = (r + g + b) / 3;
      accumulation_buffer[int(i * scale)] += gray;
      count_buffer[int(i * scale)]++;
    }
  }
};

void png_draw_callback(PNGDRAW *draw)
{
  PNGHelper *helper = (PNGHelper *)draw->pUser;
  helper->draw_callback(draw);
}
