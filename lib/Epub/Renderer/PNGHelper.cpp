#ifndef UNIT_TEST
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#else
#define vTaskDelay(t)
#define ESP_LOGE(args...)
#define ESP_LOGI(args...)
#endif
#include <PNGdec.h>
#include "PNGHelper.h"
#include "Renderer.h"

static const char *TAG = "PNG";

void convert_rgb_565_to_rgb(uint16_t rgb565, uint8_t *r, uint8_t *g, uint8_t *b)
{
  *r = ((rgb565 >> 11) & 0x1F) << 3;
  *g = ((rgb565 >> 5) & 0x3F) << 2;
  *b = (rgb565 & 0x1F) << 3;
}

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

bool PNGHelper::get_size(const std::string &path, int *width, int *height)
{
  ESP_LOGI("IMG", "Getting size of %s", m_filename.c_str());
  int rc = png.open(path.c_str(), myOpen, myClose, myRead, mySeek, NULL);
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
    ESP_LOGE(TAG, "failed to open %s %d", m_filename.c_str(), rc);
    return false;
  }
}
bool PNGHelper::render(const std::string &path, Renderer *renderer, int x_pos, int y_pos, int width, int height)
{
  this->renderer = renderer;
  this->y_pos = y_pos;
  this->x_pos = x_pos;
  int rc = png.open(path.c_str(), myOpen, myClose, myRead, mySeek, png_draw_callback);
  if (rc == PNG_SUCCESS)
  {
    this->x_scale = std::min(1.0f, float(width) / float(png.getWidth()));
    this->y_scale = std::min(1.0f, float(height) / float(png.getHeight()));
    this->last_y = -1;
    this->tmp_rgb565_buffer = (uint16_t *)malloc(png.getWidth() * 2);

    png.decode(this, 0);
    png.close();
    free(this->tmp_rgb565_buffer);
    return true;
  }
  else
  {
    ESP_LOGE(TAG, "failed to open %s %d", m_filename.c_str(), rc);
    return false;
  }
}
void PNGHelper::draw_callback(PNGDRAW *draw)
{
  // // get the rgb 565 pixel values
  png.getLineAsRGB565(draw, tmp_rgb565_buffer, 0, 0);
  // add the grayscale values to the accumulation buffer
  int y = y_pos + draw->y * y_scale;
  if (y != last_y)
  {
    for (int x = 0; x < png.getWidth() * x_scale; x++)
    {
      uint8_t r, g, b;
      convert_rgb_565_to_rgb(tmp_rgb565_buffer[int(x / x_scale)], &r, &g, &b);
      uint32_t gray = (r*38 + g*75 + b*15) >> 7;
      renderer->draw_pixel(x_pos + x, y, gray);
    }
    last_y = y;
  }
};

void png_draw_callback(PNGDRAW *draw)
{
  PNGHelper *helper = (PNGHelper *)draw->pUser;
  helper->draw_callback(draw);
}
